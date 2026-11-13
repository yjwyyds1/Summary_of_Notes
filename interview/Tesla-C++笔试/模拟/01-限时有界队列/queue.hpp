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
        (void)value;
        (void)timeout;
        // TODO
        return PushResult::timeout;
    }

    std::optional<int> pop() {
        // TODO
        return std::nullopt;
    }

    void close() {
        // TODO
    }

private:
    const std::size_t capacity_;
    // TODO: protected state and wait conditions.
};
