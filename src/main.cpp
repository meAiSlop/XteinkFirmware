#include "ha_bridge.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using namespace xteink;

namespace {

std::string redactToken(const std::string& token) {
    if (token.empty()) return "<empty>";
    if (token.size() <= 4) return "****";
    return token.substr(0, 2) + "****" + token.substr(token.size() - 2);
}

std::optional<std::string> readTextFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return std::nullopt;
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::optional<std::string> extractString(const std::string& json, const std::string& key) {
    const auto keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return std::nullopt;
    const auto colon = json.find(':', keyPos);
    const auto q1 = json.find('"', colon + 1);
    const auto q2 = json.find('"', q1 + 1);
    if (colon == std::string::npos || q1 == std::string::npos || q2 == std::string::npos) return std::nullopt;
    return json.substr(q1 + 1, q2 - q1 - 1);
}

HomeAssistantConfig loadConfig() {
    HomeAssistantConfig cfg;
    auto text = readTextFile("config/home_assistant.json");
    if (!text.has_value()) text = readTextFile("config/home_assistant.example.json");

    if (text.has_value()) {
        if (auto v = extractString(*text, "ha_base_url"); v) cfg.haBaseUrl = *v;
        if (auto v = extractString(*text, "entity_id"); v) cfg.entityId = *v;
        if (auto v = extractString(*text, "token"); v) cfg.token = *v;
        if (auto v = extractString(*text, "allowed_ssids"); v) cfg.allowedSsids = {*v};
    }

    if (const char* env = std::getenv("XTEINK_HA_TOKEN")) cfg.token = env;
    if (cfg.allowedSsids.empty()) cfg.allowedSsids = {"MyHomeWiFi"};
    if (cfg.haBaseUrl.empty()) cfg.haBaseUrl = "http://homeassistant.local:8123";
    if (cfg.entityId.empty()) cfg.entityId = "cover.living_room_blind";
    return cfg;
}

struct NetworkContext {
    std::string ssid;
    std::string bssid;
    std::string localIp;
    bool connected{false};
};

NetworkContext getNetworkContextFromDeviceSdk() {
    // TODO: Replace with actual Xteink SDK calls + connectivity callbacks.
    NetworkContext ctx;
    ctx.connected = std::getenv("XTEINK_CONNECTED") != nullptr;
    ctx.ssid = std::getenv("XTEINK_SSID") ? std::getenv("XTEINK_SSID") : "";
    ctx.bssid = std::getenv("XTEINK_BSSID") ? std::getenv("XTEINK_BSSID") : "";
    ctx.localIp = std::getenv("XTEINK_LOCAL_IP") ? std::getenv("XTEINK_LOCAL_IP") : "";
    return ctx;
}

} // namespace

int main() {
    HomeAssistantConfig cfg = loadConfig();
    HomeAssistantBridge bridge(cfg);
    NetworkContext net = getNetworkContextFromDeviceSdk();

    if (!net.connected || !bridge.isEnabledForCurrentNetwork(net.ssid, net.localIp)) {
        std::cout << "HA control disabled (untrusted, disconnected, or non-local network).\n";
        return 0;
    }

    auto state = bridge.fetchBlindState();
    if (!state.has_value()) {
        std::cout << "Unable to fetch blind state; keeping eReader functional. token=" << redactToken(cfg.token)
                  << "\n";
        return 0;
    }

    std::cout << "Blind state: " << state->state;
    if (state->currentPosition.has_value()) std::cout << " position=" << *state->currentPosition;
    std::cout << "\n";

    return 0;
}
