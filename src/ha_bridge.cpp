#include "ha_bridge.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace xteink {

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

bool HomeAssistantBridge::isEnabledForCurrentNetwork(const std::string& currentSsid,
                                                      const std::string& localIp) const {
    return isAllowedSsid(currentSsid) && isPrivateIpv4(localIp);
}

std::optional<std::string> HomeAssistantBridge::httpGet(const std::string& path) {
    // TODO: wire to Xteink HTTP client with Authorization: Bearer <token>
    // Return response body on 200, std::nullopt otherwise.
    (void)path;
    return std::nullopt;
}

std::optional<std::string> HomeAssistantBridge::httpPost(const std::string& path, const std::string& payload) {
    // TODO: wire to Xteink HTTP client with timeout + retries from cfg_.
    (void)path;
    (void)payload;
    return std::nullopt;
}

std::optional<BlindState> HomeAssistantBridge::fetchBlindState() {
    const std::string path = "/api/states/" + cfg_.entityId;
    auto body = httpGet(path);
    if (!body.has_value()) {
        return std::nullopt;
    }

    // TODO: parse JSON; example fields:
    //  state: "open"/"closed"
    //  attributes.current_position: 0..100
    BlindState state;
    state.state = "unknown";
    return state;
}

bool HomeAssistantBridge::postService(const std::string& serviceName) {
    const std::string path = "/api/services/cover/" + serviceName;
    const std::string payload = std::string("{\"entity_id\":\"") + cfg_.entityId + "\"}";
    return httpPost(path, payload).has_value();
}

bool HomeAssistantBridge::openBlind() { return postService("open_cover"); }

bool HomeAssistantBridge::closeBlind() { return postService("close_cover"); }

} // namespace xteink
