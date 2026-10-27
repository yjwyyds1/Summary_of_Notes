---
date: 2026-10-27
type: 示例代码阅读版
id: CON-CAS-001
---

# 04_cas_max · compare_exchange、失败重试与竞争

先读：[[学习/2026-10-27/04-CAS与重试|专题详解]]。

## 与 cpp 逐字一致的完整注释源码

```cpp
// 目的：用 compare_exchange_weak 维护单个原子的最大值。
// 前提：只维护整数本身，不借它发布别的非原子对象。
// 观察：候选 17、42、30、8 中最终最大值为 42。
#include <atomic>     // std::atomic<int>、CAS、relaxed。
#include <iostream>   // std::cout。
#include <thread>     // std::thread、join。

void update_max(std::atomic<int>& peak, int candidate) {
    int observed = peak.load(std::memory_order_relaxed); // 本次看到的旧值。
    while (observed < candidate &&
           !peak.compare_exchange_weak(observed, candidate,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed)) {
        // 失败时 observed 已被写成当前值；weak 也可能虚假失败。
        // 回到条件重新判断；不需要在循环体里手工 load。
    }
}

int main() {
    std::atomic<int> peak{0};
    std::thread a([&] { update_max(peak, 17); update_max(peak, 42); });
    std::thread b([&] { update_max(peak, 30); update_max(peak, 8); });
    a.join(); b.join();
    std::cout << "peak=" << peak.load(std::memory_order_relaxed) << '\n';
}
```

## 看完后核对

正常运行预期 `peak=42`；线程执行先后不固定。

- `observed = peak.load(relaxed)` 得到初始候选比较基准。
- 只在 `observed < candidate` 时进入 CAS 循环。
- CAS 成功写入 candidate；失败会更新 observed，循环重新判断。
- 两个工作线程 join 后打印最大值，示例没有把它当消息发布标志。

**变式：**若两个线程先后尝试写入 42 与 30，分别从 10 起步，画出两种先成功顺序并解释最终为何都是 42。
