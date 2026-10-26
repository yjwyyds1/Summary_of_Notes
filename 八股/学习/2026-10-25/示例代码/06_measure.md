# 06_measure · 怎样测吞吐与尾延迟

← [[学习/学习导航/2026-10-25-学习导航|学习导航]]

**用途**：可在 Obsidian 中读完整注释源码；同名 `.cpp` 可独立编译。基线 C++17。

```cpp
// 目的：用离线延迟样本演示 p50/p99 的一种明确的最近秩计算方式。
// 前置：这里的数据是教学样本，不是实测系统延迟。
#include <algorithm> // sort：排序样本。
#include <cassert>   // assert：验证分位数。
#include <cmath>     // ceil：计算最近秩索引。
#include <iostream>  // cout：输出结果。
#include <vector>    // vector：保存所有观测值。

int percentile(std::vector<int> samples, double p) { // 按值传参，排序不改调用者。
    assert(!samples.empty() && p > 0 && p <= 1); // 本教学函数的输入条件。
    std::sort(samples.begin(), samples.end());
    auto rank = static_cast<std::size_t>(std::ceil(p * samples.size())); // 秩从 1 开始。
    return samples[rank - 1]; // 转成从 0 开始的下标。
}

int main() {
    std::vector<int> micros(99, 10); // 99 个 10 微秒样本。
    micros.push_back(1000);           // 1 个长尾样本。
    assert(percentile(micros, 0.5) == 10);
    assert(percentile(micros, 0.99) == 10); // 最近秩定义下第 99 个仍为 10。
    std::cout << "p99: " << percentile(micros, 0.99)
              << ", max: " << percentile(micros, 1.0) << " us\n";
    // 真实测量还须说明采样窗口、请求起止点与分位数算法。
}
```

**观察**：核对断言与输出；涉及性能的示例仅演示语义，不以一次运行宣称速度。
