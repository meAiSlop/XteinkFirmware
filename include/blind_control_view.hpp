#pragma once

#include "ha_bridge.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace xteink {

class BlindControlView {
  public:
    using ConnectivityProvider = std::function<bool()>;

    BlindControlView(HomeAssistantBridge& bridge, ConnectivityProvider provider,
                     std::chrono::milliseconds pollInterval = std::chrono::milliseconds(1500));
    ~BlindControlView();

    void start();
    void stop();
    void render() const;

  private:
    void pollLoop();

    HomeAssistantBridge& bridge_;
    ConnectivityProvider isEnabled_;
    std::chrono::milliseconds pollInterval_;

    std::atomic<bool> running_{false};
    std::thread worker_;

    mutable std::mutex stateMutex_;
    std::optional<BlindState> lastState_;
    std::string statusMessage_{"Initializing"};
};

} // namespace xteink
