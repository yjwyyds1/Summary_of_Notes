---
date: 2026-10-27
type: 示例代码阅读版
id: CON-RELEASE-001
---

# 02_release_acquire · release/acquire 怎样发布一份数据

先读：[[学习/2026-10-27/02-release与acquire交接|专题详解]]。

## 与 cpp 逐字一致的完整注释源码

```cpp
// 目的：演示一次性消息交接的 release/acquire 同步链。
// 前提：生产者发布后不再改普通 payload；主线程等待两线程结束。
// 观察：消费者只在 acquire 读到 true 后读取两个普通字段。
#include <atomic>     // std::atomic<bool>：线程之间的发布标志。
#include <iostream>   // std::cout：输出消费者观察到的消息。
#include <thread>     // std::thread、std::this_thread::yield。

struct Message { int price{0}; int qty{0}; }; // 普通对象，不是 atomic。

int main() {
    Message message;
    std::atomic<bool> ready{false};
    std::thread producer([&] { // 按引用捕获主线程中仍存活的对象。
        message.price = 42; // 先完成普通写。
        message.qty = 3;
        ready.store(true, std::memory_order_release); // 再发布完成信号。
    });
    std::thread consumer([&] {
        while (!ready.load(std::memory_order_acquire))
            std::this_thread::yield(); // 教学演示；生产服务应设计等待策略。
        // acquire 读到发布的 true 后，这里才能读普通字段。
        std::cout << "price=" << message.price << " qty=" << message.qty << '\n';
    });
    producer.join();
    consumer.join();
}
```

## 看完后核对

正常运行预期 `price=42 qty=3`。输出本身不证明任意错误内存序也正确；正确性来自上述同步关系。

- `Message message` 在主线程创建，两个工作线程运行期间都能访问到它。
- 生产者先给 `price` 和 `qty` 赋值，再 release 存储 `ready=true`。
- 消费者在 acquire 读到 true 前只读原子变量，并通过 `yield` 减少教学循环的空转压力。
- 读到 true 后才访问普通字段；主线程 `join` 两边后结束资源寿命。

**变式：**将消费者的 payload 读取移动到 `ready.load(acquire)` 之前，指出哪一条跨线程先后链断了。
