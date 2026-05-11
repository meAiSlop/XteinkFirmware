#include "ha_bridge.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <curl/curl.h>
#include "mini_json.hpp"
#include <sstream>

namespace xteink {
namespace {

size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
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
    constexpr std::array<const char*, 7> localHosts = {"http://homeassistant.local", "https://homeassistant.local",
                                                        "http://192.168.", "http://10.", "https://192.168.",
                                                        "http://127.0.0.1", "http://localhost"};
    for (auto* prefix : localHosts) {
        if (cfg_.haBaseUrl.rfind(prefix, 0) == 0) return true;
    }
    return cfg_.haBaseUrl.rfind("http://172.", 0) == 0 || cfg_.haBaseUrl.rfind("https://172.", 0) == 0;
}

bool HomeAssistantBridge::isEnabledForCurrentNetwork(const std::string& currentSsid,
                                                      const std::string& localIp) const {
    return isAllowedSsid(currentSsid) && isPrivateIpv4(localIp) && isLocalBaseUrl();
}

std::optional<HttpResult> HomeAssistantBridge::httpRequest(const std::string& method,
                                                           const std::string& path,
                                                           const std::optional<std::string>& payload) {
    if (!isLocalBaseUrl() || cfg_.token.empty()) return std::nullopt;

    const std::string url = cfg_.haBaseUrl + path;
    for (int attempt = 0; attempt <= cfg_.maxRetries; ++attempt) {
        CURL* curl = curl_easy_init();
        if (!curl) continue;

        HttpResult result;
        struct curl_slist* headers = nullptr;
        const std::string auth = "Authorization: Bearer " + cfg_.token;
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(cfg_.httpTimeoutMs));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);

        if (payload.has_value()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload->c_str());
        }

        const CURLcode code = curl_easy_perform(curl);
        if (code != CURLE_OK) {
            result.error = curl_easy_strerror(code);
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.statusCode);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code == CURLE_OK && result.statusCode >= 200 && result.statusCode < 300) return result;
    }

    return std::nullopt;
}

std::optional<HttpResult> HomeAssistantBridge::httpGet(const std::string& path) { return httpRequest("GET", path, std::nullopt); }

std::optional<HttpResult> HomeAssistantBridge::httpPost(const std::string& path, const std::string& payload) {
    return httpRequest("POST", path, payload);
}

std::optional<BlindState> HomeAssistantBridge::fetchBlindState() {
    const std::string path = "/api/states/" + cfg_.entityId;
    auto response = httpGet(path);
    if (!response.has_value()) return std::nullopt;

    auto parsed = mini_json::parse(response->body);
    if (!parsed || !parsed->isObject()) return std::nullopt;
    const auto& root = std::get<mini_json::Object>(parsed->data);
    auto itState = root.find("state");
    if (itState == root.end() || !itState->second.isString()) return std::nullopt;

    BlindState state;
    state.state = std::get<std::string>(itState->second.data);
    auto itAttr = root.find("attributes");
    if (itAttr != root.end() && itAttr->second.isObject()) {
        const auto& attrs = std::get<mini_json::Object>(itAttr->second.data);
        auto itPos = attrs.find("current_position");
        if (itPos != attrs.end() && itPos->second.isNumber()) {
            int pos = static_cast<int>(std::get<double>(itPos->second.data));
            if (pos >= 0 && pos <= 100) state.currentPosition = pos;
        }
    }
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
