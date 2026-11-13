#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class Executor {
public:
    Executor() {
        // TODO: start exactly two workers.
    }

    ~Executor() {
        stop();
    }

    std::future<int> submit(std::function<int()> function) {
        (void)function;
        // TODO: enqueue, notify, return future.
        return {};
    }

    void stop() {
        // TODO: close, wake and join.
    }

private:
    // TODO: state and worker loop.
};
