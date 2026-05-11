#include "ha_bridge.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace xteink {
namespace {

std::string shellEscapeSingleQuoted(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    return out;
}

std::optional<std::string> extractJsonStringField(const std::string& json, const std::string& key) {
    const auto keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return std::nullopt;
    const auto colon = json.find(':', keyPos);
    if (colon == std::string::npos) return std::nullopt;
    auto firstQuote = json.find('"', colon + 1);
    if (firstQuote == std::string::npos) return std::nullopt;
    auto secondQuote = json.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return std::nullopt;
    return json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

std::optional<int> extractJsonIntField(const std::string& json, const std::string& key) {
    const auto keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return std::nullopt;
    const auto colon = json.find(':', keyPos);
    if (colon == std::string::npos) return std::nullopt;
    size_t i = colon + 1;
    while (i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
    size_t start = i;
    if (i < json.size() && (json[i] == '-' || std::isdigit(static_cast<unsigned char>(json[i])))) {
        ++i;
        while (i < json.size() && std::isdigit(static_cast<unsigned char>(json[i]))) ++i;
    } else {
        return std::nullopt;
    }
    try {
        return std::stoi(json.substr(start, i - start));
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace

HomeAssistantBridge::HomeAssistantBridge(HomeAssistantConfig config) : cfg_(std::move(config)) {}

bool HomeAssistantBridge::isAllowedSsid(const std::string& currentSsid) const {
    return std::find(cfg_.allowedSsids.begin(), cfg_.allowedSsids.end(), currentSsid) != cfg_.allowedSsids.end();
}

bool HomeAssistantBridge::isPrivateIpv4(const std::string& ip) {
    return ip.rfind("10.", 0) == 0 || ip.rfind("192.168.", 0) == 0 ||
           (ip.rfind("172.", 0) == 0 && [&]() {
               std::istringstream ss(ip.substr(4));
               int secondOctet = 0;
               char dot = '\0';
               ss >> secondOctet >> dot;
               return dot == '.' && secondOctet >= 16 && secondOctet <= 31;
           }());
}

bool HomeAssistantBridge::isLocalBaseUrl() const {
    constexpr std::array<const char*, 4> localHosts = {"http://homeassistant.local", "https://homeassistant.local",
                                                        "http://192.168.", "http://10."};
    for (auto* prefix : localHosts) {
        if (cfg_.haBaseUrl.rfind(prefix, 0) == 0) return true;
    }
    return cfg_.haBaseUrl.rfind("http://172.", 0) == 0;
}

bool HomeAssistantBridge::isEnabledForCurrentNetwork(const std::string& currentSsid,
                                                      const std::string& localIp) const {
    return isAllowedSsid(currentSsid) && isPrivateIpv4(localIp) && isLocalBaseUrl();
}

std::optional<std::string> HomeAssistantBridge::httpRequest(const std::string& method,
                                                            const std::string& path,
                                                            const std::optional<std::string>& payload) {
    if (!isLocalBaseUrl() || cfg_.token.empty()) return std::nullopt;
    const std::string url = cfg_.haBaseUrl + path;
    const std::string auth = "Authorization: Bearer " + cfg_.token;

    for (int attempt = 0; attempt <= cfg_.maxRetries; ++attempt) {
        std::string cmd = "curl -sS --max-time " + std::to_string((cfg_.httpTimeoutMs + 999) / 1000) +
                          " -X " + method + " -H '" + shellEscapeSingleQuoted(auth) +
                          "' -H 'Content-Type: application/json' '" + shellEscapeSingleQuoted(url) + "'";
        if (payload.has_value()) {
            cmd += " --data '" + shellEscapeSingleQuoted(payload.value()) + "'";
        }

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) continue;
        std::string response;
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            response += buffer;
        }
        const int rc = pclose(pipe);
        if (rc == 0 && !response.empty()) return response;
    }

    return std::nullopt;
}

std::optional<std::string> HomeAssistantBridge::httpGet(const std::string& path) { return httpRequest("GET", path, std::nullopt); }

std::optional<std::string> HomeAssistantBridge::httpPost(const std::string& path, const std::string& payload) {
    return httpRequest("POST", path, payload);
}

std::optional<BlindState> HomeAssistantBridge::fetchBlindState() {
    const std::string path = "/api/states/" + cfg_.entityId;
    auto body = httpGet(path);
    if (!body.has_value()) {
        return std::nullopt;
    }

    auto stateValue = extractJsonStringField(*body, "state");
    if (!stateValue.has_value()) return std::nullopt;

    BlindState state;
    state.state = *stateValue;
    auto pos = extractJsonIntField(*body, "current_position");
    if (pos.has_value() && *pos >= 0 && *pos <= 100) state.currentPosition = *pos;
    return state;
}

std::string HomeAssistantBridge::makeServicePayload() const {
    return std::string("{\"entity_id\":\"") + cfg_.entityId + "\"}";
}

bool HomeAssistantBridge::postService(const std::string& serviceName) {
    const std::string path = "/api/services/cover/" + serviceName;
    return httpPost(path, makeServicePayload()).has_value();
}

bool HomeAssistantBridge::openBlind() { return postService("open_cover"); }

bool HomeAssistantBridge::closeBlind() { return postService("close_cover"); }

} // namespace xteink
