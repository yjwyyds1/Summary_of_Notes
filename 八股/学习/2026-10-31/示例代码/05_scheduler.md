---
date: 2026-10-31
id: OS-SCHED-001
type: 注释源码阅读版
standard: C++17
---

# 05_scheduler.cpp：手推 RR 的每一个时间片

用途：用一个可重复的整数模拟，看清“首次运行”和“全部完成”为什么不是同一时刻。它不创建线程，不改变 OS 策略，也不模拟真实切换成本。

先修：[[学习/2026-10-20/00-先读懂C++代码与对象|引用、const、struct、assert]]、[[学习/2026-10-26/01-容器一览与选择|vector/deque 容器]]。机制与来源：[[学习/2026-10-31/05-CPU调度策略|CPU 调度完整详解]]。返回[[学习/学习导航/2026-10-31-学习导航|当天导航]]。

## 完整源码

以下代码与同目录 [[学习/2026-10-31/示例代码/05_scheduler.cpp|05_scheduler.cpp]] 逐字一致，C++17。

```cpp
// 用途：用整数时钟模拟 RR（时间片轮转），看“首次运行”和“完成”怎样不同。
// 先修：vector 保存对象；deque 在两端入队/出队；const & 表示只读借用。
// 模型：单 CPU，任务全在时刻 0 到达，只计算 CPU 时间，不含 I/O 或切换成本。
// 本程序没有创建线程、没有设置 OS 调度策略，不是实际 Linux 调度器。
// 步骤：建立就绪队列 -> 每次取队头运行一片 -> 未完成者回到队尾 -> 输出指标。
// 观察：A/B/C 的首次运行是 0/2/4，完成时刻却是 9/8/5。

#include <algorithm>  // std::min：本次最多运行一个时间片，也不能超过剩余量。
#include <cassert>    // assert：教学核对；编译时不要定义 NDEBUG，否则断言会关闭。
#include <cstddef>    // std::size_t：容器下标的无符号整数类型。
#include <cstdint>    // std::int64_t：用较宽整数记录累计模拟时刻。
#include <deque>      // std::deque：保存“下一位轮到谁”的下标队列。
#include <iostream>   // std::cout：输出模拟时间线，并非真实运行耗时。
#include <stdexcept>  // std::invalid_argument：拒绝不合法的教学输入。
#include <vector>     // std::vector<T>：尖括号中的 T 指容器保存的元素类型。

struct Job {
    char name;   // 仅供显示的任务名；调度时用下标区分任务。
    int burst;   // 总共需要多少单位 CPU 时间，必须大于 0。
};

struct Slice {
    std::size_t task;       // 本片属于 jobs 中的哪个任务。
    std::int64_t begin;     // 半开时间区间 [begin, end) 的开始。
    std::int64_t end;       // 到达 end 时，这一片已经运行完毕。
};

struct Result {
    std::vector<std::int64_t> first;   // -1 表示这个任务还从未运行。
    std::vector<std::int64_t> finish;  // -1 表示这个任务尚未完成。
    std::vector<Slice> timeline;       // 依次保存每一片，便于手算核对。
};

// jobs 是只读引用，不复制输入；quantum 是每次最多使用的 CPU 时间。
// 返回值拥有结果数据。函数只适合规模很小的教学模拟，不用于生产调度。
Result round_robin(const std::vector<Job>& jobs, int quantum) {
    if (quantum <= 0) {
        throw std::invalid_argument("quantum must be positive");
    }
    Result result;
    result.first.assign(jobs.size(), -1);  // 每个任务都分到一个初始标记。
    result.finish.assign(jobs.size(), -1);
    std::vector<int> remaining;           // 剩余 CPU 时间，与 jobs 的下标对应。
    std::deque<std::size_t> ready;        // 队列中只放下标，不复制整个任务。
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        if (jobs[i].burst <= 0) {
            throw std::invalid_argument("burst must be positive");
        }
        remaining.push_back(jobs[i].burst);
        ready.push_back(i);  // 同时到达时，用输入顺序 A、B、C 打破平局。
    }

    std::int64_t now = 0;    // 模拟时钟；没有调用 sleep，也不读取墙上时钟。
    while (!ready.empty()) {
        const std::size_t i = ready.front();
        ready.pop_front();  // 当前任务先离开就绪队列，转成“正在运行”。
        if (result.first[i] == -1) {
            result.first[i] = now;  // 只记第一次；后续轮转不能覆盖它。
        }
        const int run = std::min(quantum, remaining[i]);
        result.timeline.push_back({i, now, now + run});  // 花括号按成员顺序初始化。
        now += run;          // 时间只增加真正执行的量，最后一片可能不足 quantum。
        remaining[i] -= run;
        if (remaining[i] == 0) {
            result.finish[i] = now;  // 完成者不再入队，否则会永远转下去。
        } else {
            ready.push_back(i);     // 没做完：让其他已就绪任务先轮到 CPU。
        }
    }
    return result;
}

int main() {
    const std::vector<Job> jobs{{'A', 5}, {'B', 3}, {'C', 1}};
    const Result result = round_robin(jobs, 2);

    // 先核对手算结果，再打印。双括号使 assert 宏不会误分 initializer_list 中的逗号。
    assert((result.first == std::vector<std::int64_t>{0, 2, 4}));
    assert((result.finish == std::vector<std::int64_t>{9, 8, 5}));
    const std::vector<Slice> expected{{0, 0, 2}, {1, 2, 4}, {2, 4, 5},
                                      {0, 5, 7}, {1, 7, 8}, {0, 8, 9}};
    assert(result.timeline.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(result.timeline[i].task == expected[i].task);
        assert(result.timeline[i].begin == expected[i].begin);
        assert(result.timeline[i].end == expected[i].end);
    }
    assert(round_robin({}, 2).timeline.empty());       // 没任务时应立刻结束。
    assert(round_robin({{'X', 1}}, 5).finish[0] == 1); // 不足一个时间片也只运行 1。
    bool rejected = false;
    try {
        round_robin(jobs, 0);  // 零时间片会让模拟无法前进，所以明确拒绝。
    } catch (const std::invalid_argument&) {  // const & 只读借用异常对象。
        rejected = true;
    }
    assert(rejected);

    std::cout << "RR quantum=2\n";
    for (const Slice& slice : result.timeline) {  // & 借用已有片段，避免逐项复制。
        std::cout << jobs[slice.task].name << " [" << slice.begin
                  << "," << slice.end << ")\n";
    }
    std::cout << "task response turnaround waiting\n";
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        // 全部到达时刻为 0，因此 response=first、turnaround=finish。
        // 只有“无 I/O、无切换成本”的本模型里，waiting=turnaround-burst。
        std::cout << jobs[i].name << ' ' << result.first[i] << ' '
                  << result.finish[i] << ' ' << result.finish[i] - jobs[i].burst << '\n';
    }
    std::cout << "checks passed\n";
}
```

## 读码顺序

1. `Job` 保存原始需求，`remaining` 保存执行后的剩余量；不改原输入才能正确计算等待时间。
2. `ready` 只保存下标。队头取出、运行 `min(quantum,remaining)`、未完成放回队尾，是完整的一轮。
3. `first` 只在 -1 时填写；`finish` 只在剩余量归零时填写。若把这两处混为一谈，就会把最后一次执行误当作响应。
4. 本模型到达时刻都是 0，所以响应就是 `first`，周转就是 `finish`。等待需要累计，不只是第一次开始前的时长。
5. 断言来自独立手算，还覆盖空任务、最后一片不足 q、拒绝 q=0；编译时不要定义 `NDEBUG`。

## 预期输出与解释

```text
RR quantum=2
A [0,2)
B [2,4)
C [4,5)
A [5,7)
B [7,8)
A [8,9)
task response turnaround waiting
A 0 9 4
B 2 8 5
C 4 5 4
checks passed
```

`[begin,end)` 的长度是 `end-begin`。C 只需 1 单位，因此 `[4,5)` 后就完成；它不会继续占到 6。A 从 0 开始，却在两次中途排队中等了 3+1=4 单位，所以第一行指标是 `0 9 4`。

这份预期由程序规则推出，实际运行记录见当天 `示例验证.md`。数值是模拟单位，不是微秒或真实线程调度耗时。

Linux/g++ 或兼容 C++17 环境中的命令示例：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic 05_scheduler.cpp -o 05_scheduler
./05_scheduler
```

自测：把 q 改成 10 后先在纸上写时间线，再改输入和相应断言；原断言故意只针对 q=2，不应原样保留后期待通过。
