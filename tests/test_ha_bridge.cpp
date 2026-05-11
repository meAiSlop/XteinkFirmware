#include "ha_bridge.hpp"

#include <cassert>

int main() {
    using namespace xteink;

    HomeAssistantConfig cfg;
    cfg.entityId = "cover.kitchen";
    cfg.allowedSsids = {"A", "B"};
    HomeAssistantBridge bridge(cfg);

    assert(bridge.isAllowedSsid("A"));
    assert(!bridge.isAllowedSsid("C"));

    assert(HomeAssistantBridge::isPrivateIpv4("10.0.0.1"));
    assert(HomeAssistantBridge::isPrivateIpv4("192.168.0.50"));
    assert(HomeAssistantBridge::isPrivateIpv4("172.16.1.4"));
    assert(!HomeAssistantBridge::isPrivateIpv4("8.8.8.8"));

    assert(bridge.makeServicePayload() == "{\"entity_id\":\"cover.kitchen\"}");

    return 0;
}
