#include "blind_control_view.hpp"
#include "device_sdk.hpp"
#include "ha_bridge.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include "mini_json.hpp"
#include <optional>
#include <string>
#include <thread>

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

HomeAssistantConfig loadConfig() {
    HomeAssistantConfig cfg;
    auto text = readTextFile("config/home_assistant.json");
    if (!text.has_value()) text = readTextFile("config/home_assistant.example.json");

    if (text.has_value()) {
        auto parsed = mini_json::parse(*text);
        if (parsed && parsed->isObject()) {
            const auto& obj = std::get<mini_json::Object>(parsed->data);
            auto readString = [&](const char* key, std::string& target) {
                auto it = obj.find(key);
                if (it != obj.end() && it->second.isString()) target = std::get<std::string>(it->second.data);
            };
            readString("ha_base_url", cfg.haBaseUrl);
            readString("entity_id", cfg.entityId);
            readString("token", cfg.token);
            auto it = obj.find("allowed_ssids");
            if (it != obj.end() && it->second.isArray()) {
                for (const auto& entry : std::get<mini_json::Array>(it->second.data)) {
                    if (entry.isString()) cfg.allowedSsids.push_back(std::get<std::string>(entry.data));
                }
            }
        }
    }

    if (const char* env = std::getenv("XTEINK_HA_TOKEN")) cfg.token = env;
    if (cfg.allowedSsids.empty()) cfg.allowedSsids = {"MyHomeWiFi"};
    if (cfg.haBaseUrl.empty()) cfg.haBaseUrl = "http://homeassistant.local:8123";
    if (cfg.entityId.empty()) cfg.entityId = "cover.living_room_blind";
    return cfg;
}

} // namespace

int main() {
    HomeAssistantConfig cfg = loadConfig();
    HomeAssistantBridge bridge(cfg);
    XteinkDeviceSdk sdk;

    auto isEnabled = [&]() {
        NetworkContext net = sdk.currentNetwork();
        return net.connected && bridge.isEnabledForCurrentNetwork(net.ssid, net.localIp);
    };

    if (!isEnabled()) {
        std::cout << "HA control disabled (untrusted, disconnected, or non-local network).\n";
        return 0;
    }

    BlindControlView view(bridge, isEnabled);
    view.start();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    view.render();

    auto state = bridge.fetchBlindState();
    if (!state.has_value()) {
        std::cout << "Unable to fetch blind state; keeping eReader functional. token=" << redactToken(cfg.token)
                  << "\n";
    }

    view.stop();
    return 0;
}
