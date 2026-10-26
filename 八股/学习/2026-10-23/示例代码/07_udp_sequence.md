---
date: 2026-10-23
type: C++注释源码阅读版
cpp_standard: C++17
source: 07_udp_sequence.cpp
---

# 07_udp_sequence · 注释源码阅读版

← [[学习/学习导航/2026-10-23-学习导航|今日学习导航]] · [[学习/2026-10-23/07-练习-UDP序号缺口|对应详解]] · [[学习/2026-10-23/示例代码/07_udp_sequence.cpp|源码文件]]

下面是与 .cpp 逐字一致的完整源码；先读顶部目的与前置，再按注释追每个关键变量。

```cpp
// C++17 小练习答案：在观察窗口结束后找 UDP 行情序号的缺口。
// 没看到某个序号只表示“在本观察窗口内未收到”；可能延迟、乱序或确实丢失。
#include <cassert>  // assert：验证样例。
#include <iostream> // cout：打印缺口。
#include <set>      // set：自动去重，允许重复到达的序号只算一次。
#include <vector>   // vector：保存样例到达顺序和结果。

std::vector<int> missing_after_window(const std::vector<int>& arrivals,
                                      int first, int last) {
    // first..last 是业务方期望的闭区间；本题自定 first<=last。
    std::set<int> seen; // set 可查某个序号是否在观察期内出现。
    for (int sequence : arrivals) {
        if (sequence >= first && sequence <= last) seen.insert(sequence);
    }
    std::vector<int> missing;
    for (int sequence = first;; ++sequence) {
        if (seen.count(sequence) == 0) missing.push_back(sequence);
        if (sequence == last) break; // 到闭区间终点就停，避免 int 再加一溢出。
    }
    return missing;
}

int main() {
    // 102 先于 101 到达；102 重复一次；103 到窗口结束仍未出现。
    const std::vector<int> arrivals{100, 102, 101, 102, 104};
    const auto missing = missing_after_window(arrivals, 100, 104);
    assert(missing.size() == 1 && missing[0] == 103);
    std::cout << "not observed by deadline: " << missing[0] << '\n';
}

```

阅读后回到 [[八股测试/2026-10-23|测试页]]，先口述再展开答案。
