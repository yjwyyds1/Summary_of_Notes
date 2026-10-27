// 目的：展示 relaxed 适合不承担消息发布职责的独立统计量。
// 前提：每次加一都是 fetch_add；主线程在 join 后读最终结果。
// 观察：两个线程共执行 2000 次原子加一。
#include <atomic>     // std::atomic 与 memory_order_relaxed。
#include <iostream>   // std::cout。
#include <thread>     // std::thread、join。

int main() {
    std::atomic<int> events{0}; // 只统计次数，不保护别的普通字段。
    auto count = [&events] { // 引用捕获，主线程保证 events 寿命覆盖两任务。
        for (int i = 0; i < 1000; ++i)
            events.fetch_add(1, std::memory_order_relaxed);
    };
    std::thread left(count), right(count);
    left.join();  // 工作线程的完成与成功返回 join 建立同步。
    right.join(); // 两边完成后才观察最终计数。
    std::cout << "events=" << events.load(std::memory_order_relaxed) << '\n';
}
