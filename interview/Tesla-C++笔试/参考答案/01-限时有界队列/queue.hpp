#pragma once
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

enum class PushResult { accepted, timeout, closed };

class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("capacity");
    }

    PushResult pushFor(int value, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!space_.wait_for(lock, timeout, [&] {
                return closed_ || queue_.size() < capacity_;
            })) return PushResult::timeout;
        if (closed_) return PushResult::closed;
        try {
            queue_.push(value);
        } catch (...) {
            // The free slot still exists. Give another writer a chance.
            lock.unlock();
            space_.notify_one();
            throw;
        }
        lock.unlock();
        ready_.notify_one();
        return PushResult::accepted;
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return std::nullopt;
        const int value = queue_.front();
        queue_.pop();
        lock.unlock();
        space_.notify_one();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        ready_.notify_all();
        space_.notify_all();
    }

private:
    const std::size_t capacity_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::condition_variable space_;
    std::queue<int> queue_;
    bool closed_ = false;
};
