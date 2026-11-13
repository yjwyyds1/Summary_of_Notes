---
date: 2026-10-31
id: OS-REPLACE-001
type: 注释源码阅读版
standard: C++17
---

# 06_page_replacement.cpp：FIFO 与 Clock 的小型模拟

用途：固定页号访问序列与页框数量，观察缺页、FIFO 装入顺序和 Clock 引用位。代码不访问真实页表，不产生可对应的真实 Linux 页面回收记录，也没有模拟磁盘与脏页成本。

先修：[[学习/2026-10-24/03-虚拟内存|页与页表]]、[[学习/2026-10-26/01-容器一览与选择|vector/deque]]、[[学习/2026-10-20/00-先读懂C++代码与对象|引用与对象]]。机制、手算表与来源：[[学习/2026-10-31/06-页面置换策略|页面置换详解]]。返回[[学习/学习导航/2026-10-31-学习导航|当天导航]]。

## 完整源码

以下代码与同目录 [[学习/2026-10-31/示例代码/06_page_replacement.cpp|06_page_replacement.cpp]] 逐字一致，C++17。

```cpp
// 用途：模拟 FIFO 与 Clock 的受害页选择；用已知序列核对缺页次数。
// 先修：vector/deque、循环、bool；这里只保存整数页号，不操作真实页表。
// 模型：页框数固定；初始全空；每次访问一个页；未驻留就计一次模拟缺页。
// 不模拟脏页、磁盘、共享页、并发或预取；不是 Linux 内核的实际回收实现。
// 观察：FIFO 增加页框未必减少缺页；Clock 的引用位只有 0/1，不是精确访问排序。

#include <algorithm>  // std::find：在线性容器里查找页号。
#include <cassert>    // assert：核对手算值；不要定义 NDEBUG。
#include <cstddef>    // std::size_t：页框数量、下标和缺页计数的类型。
#include <deque>      // std::deque：从队头淘汰最早装入的页。
#include <iostream>   // std::cout：打印固定模拟结果。
#include <stdexcept>  // std::invalid_argument：拒绝零页框，避免取模除零。
#include <vector>     // std::vector：保存访问序列及固定数量页框。

// refs 是只读访问序列；capacity 是可使用的页框数；返回模拟缺页次数。
std::size_t fifo_faults(const std::vector<int>& refs, std::size_t capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("capacity must be positive");
    }
    std::deque<int> resident;  // 队头最早装入，队尾最近装入；不是最近访问顺序。
    std::size_t faults = 0;
    for (int page : refs) {
        // find 返回 end() 表示没有找到。命中时什么也不挪动，这是 FIFO 的关键。
        if (std::find(resident.begin(), resident.end(), page) != resident.end()) {
            continue;
        }
        ++faults;
        if (resident.size() == capacity) {
            resident.pop_front();  // 满了才淘汰；仍有空框时直接装入即可。
        }
        resident.push_back(page);
    }
    return faults;
}

struct Frame {
    int page = 0;          // 存放的页号；只有 occupied 为 true 才有意义。
    bool referenced = false; // 自上次清零后是否被访问过，俗称引用位 R。
    bool occupied = false;   // 用单独标记区分空框，避免拿某个合法页号当“空”。
};

struct ClockResult {
    std::size_t faults = 0;
    std::vector<Frame> frames;  // 下标就是页框号，替换只修改该框的内容。
    std::size_t hand = 0;       // 下次找受害页从这里开始，像钟表指针一样绕回。
};

ClockResult clock_replace(const std::vector<int>& refs, std::size_t capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("capacity must be positive");
    }
    ClockResult result;
    result.frames.resize(capacity);  // 创建 capacity 个 Frame，按成员默认值初始化。
    for (int page : refs) {
        bool hit = false;
        for (Frame& frame : result.frames) {  // & 表示直接修改页框，不是修改副本。
            if (frame.occupied && frame.page == page) {
                frame.referenced = true;    // 读取或写入都算访问，命中后重新置 1。
                hit = true;
                break;
            }
        }
        if (hit) {
            continue;  // 本变体在命中时不移动指针。
        }
        ++result.faults;
        // 空框直接用；R=0 的旧框可替换；R=1 则清零并给它一次第二机会。
        while (result.frames[result.hand].occupied &&
               result.frames[result.hand].referenced) {
            result.frames[result.hand].referenced = false;
            result.hand = (result.hand + 1) % capacity; // % 让末尾绕回下标 0。
        }
        result.frames[result.hand] = Frame{page, true, true};
        // 新页刚刚被访问过，所以 R=1；替换完成后指针前移一格。
        result.hand = (result.hand + 1) % capacity;
    }
    return result;
}

int main() {
    // 经典 Belady 序列：相同访问顺序、相同 FIFO 规则，只改变页框数量。
    const std::vector<int> classic{1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    const std::size_t fifo3 = fifo_faults(classic, 3);
    const std::size_t fifo4 = fifo_faults(classic, 4);
    assert(fifo3 == 9);
    assert(fifo4 == 10);

    // 这个短序列专门观察“第二机会”：最终 Clock 留住了刚访问过的页 2。
    const std::vector<int> refs{1, 2, 3, 1, 4, 2, 5};
    const ClockResult result = clock_replace(refs, 3);
    assert(result.faults == 5);
    assert(result.hand == 0);
    assert(result.frames[0].page == 4 && result.frames[0].referenced);
    assert(result.frames[1].page == 2 && !result.frames[1].referenced);
    assert(result.frames[2].page == 5 && result.frames[2].referenced);
    assert(fifo_faults({}, 3) == 0 && clock_replace({}, 3).faults == 0);
    assert(fifo_faults({7, 7, 7}, 1) == 1);
    assert(clock_replace({7, 7, 7}, 1).faults == 1);
    assert(clock_replace({1, 2, 1}, 1).faults == 3);
    bool rejected = false;
    try {
        clock_replace(refs, 0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    std::cout << "FIFO classic: frames=3 faults=" << fifo3
              << ", frames=4 faults=" << fifo4 << '\n';
    std::cout << "Clock trace:";
    for (int page : refs) {
        std::cout << ' ' << page;
    }
    std::cout << "\nClock frames:";
    for (const Frame& frame : result.frames) { // const & 表示这里只读页框。
        std::cout << ' ' << frame.page << '(' << frame.referenced << ')';
    }
    std::cout << " hand=" << result.hand << " faults=" << result.faults << '\n';
    std::cout << "checks passed\n";
}
```

## 读码顺序

1. FIFO 先 `find`，命中就直接继续。只有缺页才可能弹出队头，因此它按装入顺序，不按访问顺序排列。
2. `Frame` 有三个成员：页号、R 引用位、占用标记；空框不能仅靠页号 0 表示。
3. Clock 命中时通过 `Frame&` 修改真实元素的引用位。没有 `&` 就会只改副本，算法行为会错。
4. 找受害页时，先给 R=1 的框清零并绕到下一框；遇空框或 R=0 时停下，把新页放进去，再前移指针。
5. `capacity==0` 提前抛异常，防止后续 `% capacity` 除零。状态规模小，线性查找方便看懂，不代表生产实现的复杂度选择。

## 预期输出与解释

```text
FIFO classic: frames=3 faults=9, frames=4 faults=10
Clock trace: 1 2 3 1 4 2 5
Clock frames: 4(1) 2(0) 5(1) hand=0 faults=5
checks passed
```

第一行比较完全相同访问序列下的两种容量，展示 FIFO 的 Belady 异常。第三行按框编号输出，`2(0)` 表示框中是页 2、R 已清零，**不表示页 2 从未访问过**。它在最后扫描时获得第二机会，所以最终还留在内存里。

访问量 `n`、容量 `k` 时，本例线性查找使总时间为 `O(nk)`，状态空间为 `O(k)`。输出是固定模拟结果，实际编译运行情况见当天 `示例验证.md`。

Linux/g++ 或兼容 C++17 环境中的命令示例：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic 06_page_replacement.cpp -o 06_page_replacement
./06_page_replacement
```

自测：如果删去命中分支中的 `frame.referenced=true`，最后一次访问 5 会淘汰哪一页？先看详解手算表，再判断现有断言会在哪里发现问题。
