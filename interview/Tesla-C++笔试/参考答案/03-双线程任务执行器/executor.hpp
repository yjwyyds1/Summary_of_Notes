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
        workers_.reserve(2);
        try {
            for (int i = 0; i < 2; ++i)
                workers_.emplace_back([this] { run(); });
        } catch (...) {
            stop();
            throw;
        }
    }

    ~Executor() {
        stop();
    }

    std::future<int> submit(std::function<int()> function) {
        std::packaged_task<int()> task(std::move(function));
        auto result = task.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) throw std::runtime_error("executor stopped");
            tasks_.push(std::move(task));
        }
        ready_.notify_one();
        return result;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        ready_.notify_all();
        for (auto& worker : workers_)
            if (worker.joinable()) worker.join();
    }

private:
    void run() {
        for (;;) {
            std::packaged_task<int()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [&] { return closed_ || !tasks_.empty(); });
                if (tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task(); // The result or task exception is stored in its future.
        }
    }

    std::mutex mutex_;
    std::condition_variable ready_;
    std::queue<std::packaged_task<int()>> tasks_;
    bool closed_ = false;
    std::vector<std::thread> workers_;
};
