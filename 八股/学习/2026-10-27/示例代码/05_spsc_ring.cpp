// 目的：教学版固定容量 SPSC 环形队列，演示槽位的发布与回收。
// 前提：恰好一个生产者、一个消费者；队列对象活到两线程 join 后。
// 观察：物理 4 格留 1 格，交付 1..1000 的顺序应保持。
#include <array>      // std::array：固定的四个 int 槽位。
#include <atomic>     // std::atomic<std::size_t>：跨线程头尾位置。
#include <cstddef>    // std::size_t：数组下标类型。
#include <iostream>   // std::cout。
#include <thread>     // std::thread、yield、join。

class SpscRing {
    static constexpr std::size_t size_ = 4; // 可用容量只有 3。
    std::array<int, size_> data_{};          // 普通槽位，所有权由索引交接。
    std::atomic<std::size_t> head_{0};      // 只有消费者修改 head_。
    std::atomic<std::size_t> tail_{0};      // 只有生产者修改 tail_。
    static std::size_t next(std::size_t i) { return (i + 1) % size_; }
public:
    bool try_push(int value) { // 只能由唯一生产者线程调用。
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto after = next(tail);
        if (after == head_.load(std::memory_order_acquire)) return false;
        data_[tail] = value; // 槽位尚未发布，只有生产者可写。
        tail_.store(after, std::memory_order_release); // 发布已写好的槽位。
        return true;
    }
    bool try_pop(int& value) { // 只能由唯一消费者线程调用。
        const auto head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) return false;
        value = data_[head]; // acquire 看到发布后才读普通槽位。
        head_.store(next(head), std::memory_order_release); // 允许生产者复用。
        return true;
    }
};

int main() {
    SpscRing queue;
    std::atomic<bool> ordered{true}; // 仅在检测错误时置 false。
    std::thread producer([&] {
        for (int i = 1; i <= 1000; ++i)
            while (!queue.try_push(i)) std::this_thread::yield();
    });
    std::thread consumer([&] {
        for (int expected = 1; expected <= 1000; ++expected) {
            int value = 0;
            while (!queue.try_pop(value)) std::this_thread::yield();
            if (value != expected) ordered.store(false);
        }
    });
    producer.join(); consumer.join();
    std::cout << "ordered=" << ordered.load() << '\n';
}
