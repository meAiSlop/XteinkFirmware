#include "ha_bridge.hpp"

#include <iostream>

using namespace xteink;

int main() {
    HomeAssistantConfig cfg{
        .haBaseUrl = "http://homeassistant.local:8123",
        .entityId = "cover.living_room_blind",
        .token = "<securely-load-from-keystore>",
        .allowedSsids = {"MyHomeWiFi"},
        .httpTimeoutMs = 3000,
        .maxRetries = 2,
    };

    HomeAssistantBridge bridge(cfg);

    // TODO: integrate with actual device networking APIs.
    const std::string currentSsid = "MyHomeWiFi";
    const std::string localIp = "192.168.1.55";

    if (!bridge.isEnabledForCurrentNetwork(currentSsid, localIp)) {
        std::cout << "HA control disabled (untrusted or non-local network).\n";
        return 0;
    }

    auto state = bridge.fetchBlindState();
    if (!state.has_value()) {
        std::cout << "Unable to fetch blind state; keeping eReader fully functional.\n";
        return 0;
    }

    std::cout << "Blind state: " << state->state << "\n";

    // In UI, wire these to button taps.
    // bridge.openBlind();
    // bridge.closeBlind();

    return 0;
}
