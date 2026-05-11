#include "blind_control_view.hpp"

#include <iostream>

namespace xteink {

BlindControlView::BlindControlView(HomeAssistantBridge& bridge,
                                   ConnectivityProvider provider,
                                   std::chrono::milliseconds pollInterval)
    : bridge_(bridge), isEnabled_(std::move(provider)), pollInterval_(pollInterval) {}

BlindControlView::~BlindControlView() { stop(); }

void BlindControlView::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&BlindControlView::pollLoop, this);
}

void BlindControlView::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

void BlindControlView::pollLoop() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (!isEnabled_()) {
                statusMessage_ = "Disabled (untrusted/disconnected)";
                lastState_.reset();
            } else if (auto state = bridge_.fetchBlindState(); state.has_value()) {
                statusMessage_ = "Connected";
                lastState_ = *state;
            } else {
                statusMessage_ = "HA unavailable";
                lastState_.reset();
            }
        }
        std::this_thread::sleep_for(pollInterval_);
    }
}

void BlindControlView::render() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    std::cout << "[BlindControlView] status=" << statusMessage_;
    if (lastState_.has_value()) {
        std::cout << " state=" << lastState_->state;
        if (lastState_->currentPosition.has_value()) std::cout << " pos=" << *lastState_->currentPosition;
    }
    std::cout << "\n";
}

} // namespace xteink
