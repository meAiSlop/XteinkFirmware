#pragma once

#include <optional>
#include <string>
#include <vector>

namespace xteink {

struct HomeAssistantConfig {
    std::string haBaseUrl;
    std::string entityId;
    std::string token;
    std::vector<std::string> allowedSsids;
    int httpTimeoutMs{3000};
    int maxRetries{2};
};

struct BlindState {
    std::string state; // "open", "closed", etc.
    std::optional<int> currentPosition; // 0..100
};

class HomeAssistantBridge {
  public:
    explicit HomeAssistantBridge(HomeAssistantConfig config);

    bool isEnabledForCurrentNetwork(const std::string& currentSsid,
                                    const std::string& localIp) const;

    std::optional<BlindState> fetchBlindState();
    bool openBlind();
    bool closeBlind();

    // Test/validation helpers.
    static bool isPrivateIpv4(const std::string& ip);
    bool isAllowedSsid(const std::string& currentSsid) const;
    std::string makeServicePayload() const;

  private:
    HomeAssistantConfig cfg_;

    bool postService(const std::string& serviceName);
    std::optional<std::string> httpGet(const std::string& path);
    std::optional<std::string> httpPost(const std::string& path, const std::string& payload);

    bool isLocalBaseUrl() const;
    std::optional<std::string> httpRequest(const std::string& method,
                                           const std::string& path,
                                           const std::optional<std::string>& payload);
};

} // namespace xteink
