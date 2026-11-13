#pragma once
#include <functional>
#include <memory>
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
        auto next = callback ? std::make_shared<const Callback>(std::move(callback)) : nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback_.swap(next);
        }
        // A replaced callback and its captures are destroyed outside the lock.
    }

    DeviceState snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    bool setVolume(int volume) {
        if (volume < 0 || volume > 100) return false;
        DeviceState committed;
        std::shared_ptr<const Callback> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.volume = volume;
            ++state_.revision;
            committed = state_;
            callback = callback_;
        }
        if (callback) (*callback)(committed);
        return true;
    }

private:
    mutable std::mutex mutex_;
    DeviceState state_;
    std::shared_ptr<const Callback> callback_;
};
