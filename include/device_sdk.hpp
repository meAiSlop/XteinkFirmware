#pragma once

#include <cstdlib>
#include <string>

namespace xteink {

struct NetworkContext {
    std::string ssid;
    std::string bssid;
    std::string localIp;
    bool connected{false};
};

class DeviceSdk {
  public:
    virtual ~DeviceSdk() = default;
    virtual NetworkContext currentNetwork() const = 0;
};

class XteinkDeviceSdk : public DeviceSdk {
  public:
    NetworkContext currentNetwork() const override {
        // Replace with actual Xteink SDK API calls in firmware integration layer.
        NetworkContext ctx;
        ctx.connected = std::getenv("XTEINK_CONNECTED") != nullptr;
        ctx.ssid = std::getenv("XTEINK_SSID") ? std::getenv("XTEINK_SSID") : "";
        ctx.bssid = std::getenv("XTEINK_BSSID") ? std::getenv("XTEINK_BSSID") : "";
        ctx.localIp = std::getenv("XTEINK_LOCAL_IP") ? std::getenv("XTEINK_LOCAL_IP") : "";
        return ctx;
    }
};

} // namespace xteink
