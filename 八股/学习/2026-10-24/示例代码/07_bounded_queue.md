# 07_bounded_queue — 完整注释源码

← [[学习/2026-10-24/示例代码/README|代码索引]] · [[学习/2026-10-24/07-练习-有界队列|主题详解]]

以下代码与同名 .cpp 逐字一致；完整编译与输出见 [[学习/2026-10-24/示例验证]]。

```cpp
// C++17 小练习答案：容量为 2 的整数任务队列，满时生产者等待，空时消费者等待。
// close 后拒绝新任务，消费者先取完已入队任务，再收到结束信号。
#include <cassert>            // assert：核对接收顺序。
#include <condition_variable> // 两个条件变量分别等待“有数据”和“有空位”。
#include <deque>              // deque：从队头取、队尾放。
#include <iostream>           // cout：展示已消费的任务。
#include <mutex>              // mutex/unique_lock：保护队列和关闭状态。
#include <optional>           // optional：无任务且已关闭时明确返回“没有值”。
#include <stdexcept>          // runtime_error：关闭后拒绝入队。
#include <thread>             // thread：运行消费者。
#include <vector>             // vector：记录最终处理顺序。

class BoundedQueue {
    const std::size_t capacity_;      // 队列上限，构造时保证大于 0。
    std::deque<int> items_;           // 等待处理的整数任务。
    bool closed_ = false;             // 关闭后不再接收新任务。
    std::mutex mutex_;                // items_ 和 closed_ 只能在持锁时访问。
    std::condition_variable not_empty_; // 队列有数据或被关闭时唤醒消费者。
    std::condition_variable not_full_;  // 队列有空位或被关闭时唤醒生产者。
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("capacity must be positive");
    }
    void push(int item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [&] { return closed_ || items_.size() < capacity_; });
        if (closed_) throw std::runtime_error("queue closed");
        items_.push_back(item); // 只在有空位且未关闭时放入。
        lock.unlock();          // 通知前放开锁，让消费者立刻有机会取得它。
        not_empty_.notify_one();
    }
    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [&] { return closed_ || !items_.empty(); });
        if (items_.empty()) return std::nullopt; // 已关闭且已取完。
        const int item = items_.front();
        items_.pop_front();
        lock.unlock();
        not_full_.notify_one(); // 刚腾出一个空位，生产者可以继续。
        return item;
    }
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        not_empty_.notify_all(); // 唤醒空队列上的消费者。
        not_full_.notify_all();  // 唤醒满队列上的生产者以便它们退出。
    }
};

int main() {
    BoundedQueue queue(2);
    std::vector<int> consumed; // 只有消费者写，join 后主线程才读。
    std::thread consumer([&] {
        while (const auto item = queue.pop()) consumed.push_back(*item);
    });
    queue.push(1);
    queue.push(2);
    queue.push(3); // 满了就等待，不能无限增长。
    queue.close();
    consumer.join();
    assert((consumed == std::vector<int>{1, 2, 3}));
    for (int item : consumed) std::cout << item << ' ';
    std::cout << '\n';
}
```
