---
date: 2026-10-27
type: 示例代码阅读版
id: CON-RELAXED-001
---

# 03_relaxed_counter · relaxed 计数能保证什么，不能保证什么

先读：[[学习/2026-10-27/03-relaxed计数与发布边界|专题详解]]。

## 与 cpp 逐字一致的完整注释源码

```cpp
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
```

## 看完后核对

正常运行预期 `events=2000`；这不演示 relaxed 发布任何普通数据。

- 定义唯一的原子 `events`；没有普通 payload 被它发布。
- 两个线程分别做 1000 次 `fetch_add(1, relaxed)`。
- 主线程等待两个 `join()`，再调用 `load(relaxed)`。
- 最终值用于功能核对，不用于推断运行中每一时刻观察顺序。

**变式：**假设生产者先写 `payload=7` 后 relaxed 增加 `events`，消费者看到 `events>0` 就读普通 `payload`，指出缺失的同步边。
