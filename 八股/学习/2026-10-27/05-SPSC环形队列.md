---
date: 2026-10-27
type: 学习专题
cpp_standard: C++17
id: CON-SPSC-001
learning_status: 待学习
---

# 单生产者单消费者 SPSC 环形队列

← [[学习/学习导航/2026-10-27-学习导航|今日导航]] · [[八股测试/2026-10-27|今日自测]]

> [!abstract] 学完应能回答
> 固定容量 SPSC 队列如何判断空/满？为什么发布尾指针与读取对方指针要使用 release/acquire？

## 名词先说清楚

SPSC = single producer, single consumer，只有一个入队者和一个出队者。环形缓冲区是固定数组，下标到尾端后绕回开头；`head` 指向下一次出队，`tail` 指向下一次入队。‘留一格’用一个空槽区分头尾相等时的空和满。

## 要解决的实际问题

行情接入线程把已解析消息交给单个处理线程；10/24 已用 mutex+条件变量完成有界队列。今天新增的是在**严格一进一出**前提下，如何把队列槽位的所有权通过两个原子位置交接。若前提不满足，应回到成熟并发队列或锁实现。

## 机制：按时间线追踪

生产者读取自己写的 tail（relaxed），acquire 读消费者的 head 来确认下一槽已被释放；写 `data[tail]` 后 release 发布新 tail。消费者对称：读取自己写的 head（relaxed），acquire 读 tail 确认新数据已发布；读槽后 release 更新 head。这个成对设计让生产者不会覆盖消费者仍在读的槽，消费者也不会读取尚未写完的槽。容量固定为 4，实际可用为 3；正常运行中的暂时空/满并非故障。

## 完整可运行代码

与同名 `.cpp`、Markdown 阅读版源码逐字一致。先看注释，再按下方步骤手推；危险的无同步/UAF 反例只作文字讨论，不运行。

```cpp
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
```

### 逐步读代码

1. `next(i)=(i+1)%4` 让数组位置循环，但留一格防止空/满混淆。
2. `try_push` 先用 acquire 检查 head；成功时写槽，再 release 推进 tail。
3. `try_pop` 先用 acquire 检查 tail；成功时读槽，再 release 推进 head。
4. 两个线程交付 1 到 1000；调用方在 false 时 yield 重试，最后核对 FIFO 顺序。

**输出范围：**正常运行预期 `ordered=1`；单次运行不能证明所有调度下正确，更不证明速度优于锁。

## 对象与先后关系图

```mermaid
flowchart LR
  A["生产者检查 head"] --> B["写 data[tail]"]
  B --> C["tail.store release"]
  C --> D["消费者 tail.load acquire"]
  D --> E["读 data[head]"]
  E --> F["head.store release"]
  F --> A
```

图中箭头表示本节的逻辑依赖或可见性链，不代表 CPU 必须逐条执行图中的机器指令。

## 项目里如何选择

单接收线程到单处理线程的固定容量交接可评估 SPSC。成熟项目如 folly 的 ProducerConsumerQueue 也限定一个生产者与一个消费者；读真实项目时先找角色数量、生命周期、满队列策略、关闭协议，再看性能优化。

## 反例与边界

- 有第二个生产者就可能同时写同一 tail/槽位；不是把两个索引改成 atomic 就自动支持 MPMC。
- 原子索引本身安全不等于普通槽位安全；成对 release/acquire 才建立复用顺序。
- 忙等或 yield 只演示消费过程，没有通用公平/时延上界；若生产者异常退出，消费者可能永远等待，因此真实接口还要关闭协议。
- 析构前必须停止并 join 两线程，不能让线程继续访问已销毁的槽位。

## 换条件自己推演

物理 4 格时依次 push 1、2、3、4，接着 pop 一次、push 4；写出每一步 head/tail 与成功/失败。

## 面试口述版

> SPSC 只有一进一出；留一格区分空满；先写槽再 release 推 tail，acquire 看见新 tail 才读；回收槽位对称处理。

## 来源与版本边界

**题型来源：**[腾讯 C++ 后端 WXG 二面候选人复盘](https://www.nowcoder.com/feed/main/detail/321db23766b94e739d092c126b832d29)明确记录‘SPSC 无锁队列如何保证并发安全’；作者 Monsteer714，页面显示 06-25 编辑但未给出年份。本文的容量 4 教学实现为自行设计。

**技术核对（2026-10-27 访问）：**
- [C++17 草案 [atomics.order]](https://timsong-cpp.github.io/cppwp/n4659/atomics.order)
- [folly ProducerConsumerQueue 源码](https://github.com/facebook/folly/blob/main/folly/ProducerConsumerQueue.h)
- [folly 官方队列说明](https://github.com/facebook/folly/blob/main/folly/docs/ProducerConsumerQueue.md)

语言规则按 C++17 对应草案 N4659 解释；该在线草案是工作稿，真实实现的硬件路径与性能须另测。
