#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

class BlockingQueue {
public:
    bool push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) {
                return false; // 关门后，不再收新数据。
            }
            queue_.push(value);
        } // 先把数据放好，并释放锁，再通知读线程。
        ready_.notify_one();
        return true;
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 空且没关：等待。否则回来继续判断能不能取。
        ready_.wait(lock, [this] { return closed_ || !queue_.empty(); });

        if (queue_.empty()) {
            return std::nullopt; // 能走到这里但没数据，必定已关。
        }

        int value = queue_.front(); // front 只看队首。
        queue_.pop();               // pop 才真正删除队首。
        return value;               // 包装成有值的 optional<int>。
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true; // 只关门，保留已有数据。
        }
        ready_.notify_all();
    }

private:
    std::mutex mutex_; // 同时保护 queue_ 和 closed_。
    std::condition_variable ready_;
    std::queue<int> queue_;
    bool closed_ = false;
};

int main() {
    BlockingQueue queue;

    std::thread reader([&queue] {
        while (true) {
            std::optional<int> result = queue.pop();
            if (!result.has_value()) {
                std::cout << "reader exits\n";
                break;
            }
            std::cout << "read " << *result << '\n';
        }
    });

    const bool acceptedZero = queue.push(0);
    const bool accepted42 = queue.push(42);
    queue.close();
    reader.join();

    const bool acceptedAfterClose = queue.push(99);
    assert(acceptedZero && accepted42 && !acceptedAfterClose);
    assert(!queue.pop().has_value());
}
