#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

class BlockingQueue {
public:
    bool push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) return false;
            queue_.push(value);
        }
        ready_.notify_one();
        return true;
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return std::nullopt;
        const int value = queue_.front();
        queue_.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        ready_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable ready_;
    std::queue<int> queue_;
    bool closed_ = false;
};

int main() {
    BlockingQueue queue;
    std::vector<int> consumed;
    std::thread reader([&] {
        while (auto value = queue.pop()) consumed.push_back(*value);
    });
    const bool first = queue.push(0);
    const bool second = queue.push(42);
    assert(first && second);
    queue.close();
    reader.join();
    assert((consumed == std::vector<int>{0, 42}));
    assert(!queue.push(99));
    assert(!queue.pop());
    std::cout << "blocking queue OK\n";
}
