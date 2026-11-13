#pragma once
#include <functional>
#include <mutex>
#include <utility>

struct DeviceState {
    int volume = 0;
    int revision = 0;
};

class Device {
public:
    using Callback = std::function<void(DeviceState)>;

    void setCallback(Callback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
    }

    DeviceState snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    // Existing implementation: repair it according to the specification.
    bool setVolume(int volume) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.volume = volume;
        if (volume < 0 || volume > 100) return false;
        if (callback_) callback_(state_);
        ++state_.revision;
        return true;
    }

private:
    mutable std::mutex mutex_;
    DeviceState state_;
    Callback callback_;
};
