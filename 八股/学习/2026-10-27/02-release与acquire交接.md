---
date: 2026-10-27
type: 学习专题
cpp_standard: C++17
id: CON-RELEASE-001
learning_status: 待学习
---

# release/acquire 怎样发布一份数据

← [[学习/学习导航/2026-10-27-学习导航|今日导航]] · [[八股测试/2026-10-27|今日自测]]

> [!abstract] 学完应能回答
> 生产者先写普通 payload 再把 ready 设为 true；消费者看到 ready 后读 payload，怎样保证读取有定义？

## 名词先说清楚

`release` 可先想成‘发布之前已经写好的内容’，`acquire` 可先想成‘拿到发布信号后再使用内容’。`happens-before` 是 C++ 规则认可的跨线程先后，不是墙上时钟或源码行号。`ready` 是同步载体；`payload` 本身可以是普通数据，但访问必须被这条同步链保护。

## 要解决的实际问题

行情接收线程构造好一条消息，再交给处理线程。如果处理线程只看到了‘可用’标志，却没有规则保证消息字段在它读取前已完成写入，就可能产生数据竞争。10/20 的 shared_ptr 发布的是指针所有权快照；本节独立讲普通 payload 的发布链。

## 机制：按时间线追踪

第一条线在生产者：`message.price=...` 和 `message.qty=...` 按序发生在 `ready.store(true, release)` 之前。第二条线在消费者：`ready.load(acquire)` 读到该 release 写入的 true 之后，才访问两个字段。release/acquire 在同一个 atomic 对象上形成同步边，两条线程内部顺序接起来构成完整的 happens-before。代码里只发布一次，之后生产者不再修改 payload，所以不需要额外回收协议。

## 完整可运行代码

与同名 `.cpp`、Markdown 阅读版源码逐字一致。先看注释，再按下方步骤手推；危险的无同步/UAF 反例只作文字讨论，不运行。

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

### 逐步读代码

1. `Message message` 在主线程创建，两个工作线程运行期间都能访问到它。
2. 生产者先给 `price` 和 `qty` 赋值，再 release 存储 `ready=true`。
3. 消费者在 acquire 读到 true 前只读原子变量，并通过 `yield` 减少教学循环的空转压力。
4. 读到 true 后才访问普通字段；主线程 `join` 两边后结束资源寿命。

**输出范围：**正常运行预期 `price=42 qty=3`。输出本身不证明任意错误内存序也正确；正确性来自上述同步关系。

## 对象与先后关系图

```mermaid
flowchart LR
  A["生产者写 price/qty"] --> B["ready.store true, release"]
  B -->|消费者读到这次 true| C["ready.load acquire"]
  C --> D["消费者读 price/qty"]
```

图中箭头表示本节的逻辑依赖或可见性链，不代表 CPU 必须逐条执行图中的机器指令。

## 项目里如何选择

单次构造后只读的配置快照、一次性结果通知可用这个模式解释。多次发布、覆盖和销毁要增加版本、所有权或队列协议；实践里 mutex/条件变量常更好维护。

## 反例与边界

- acquire 必须在读到相应 release 写入的值时才形成这条同步边。
- `ready` 不是计时器；忙等可能浪费 CPU，且不会因 release/acquire 自动提供实时调度保证。
- 生产者发布后继续写同一普通 payload，会破坏单次发布的前提。

## 换条件自己推演

将消费者的 payload 读取移动到 `ready.load(acquire)` 之前，指出哪一条跨线程先后链断了。

## 面试口述版

> 写完 payload 后 release 发布标志；消费者 acquire 读到该发布值后才读 payload，于是普通数据获得合法的跨线程先后。

## 来源与版本边界

**题型来源：**[FUNPLUS 趣加面经](https://www.nowcoder.com/discuss/666390918777290752)明确记录了‘用过哪些内存序’，2024-09-19；单次 payload/ready 发布场景为本库自行构造，用于解释条件。

**技术核对（2026-10-27 访问）：**
- [C++17 草案 [atomics.order]](https://timsong-cpp.github.io/cppwp/n4659/atomics.order)
- [C++17 草案 [intro.races]](https://timsong-cpp.github.io/cppwp/n4659/intro.races)

语言规则按 C++17 对应草案 N4659 解释；该在线草案是工作稿，真实实现的硬件路径与性能须另测。
