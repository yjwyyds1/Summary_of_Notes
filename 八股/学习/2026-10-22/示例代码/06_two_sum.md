---
date: 2026-10-22
cpp_standard: C++17
type: 代码阅读版
source: 06_two_sum.cpp
---

# 06_two_sum · 先查差值，再登记当前值

← [[学习/学习导航/2026-10-22-学习导航|学习导航]] · [[学习/2026-10-22/06-两数之和与哈希查找|逐步讲解]] · [[学习/2026-10-22/示例代码/06_two_sum.cpp|可编译源码]]

问题要返回两个不同下标。本例用 `std::optional` 明确表达无解，用哈希表保存已经经过的值。完整代码与同名 cpp 一致。

```cpp
// C++17 学习示例：数组里找两个不同下标，其元素之和等于 target。
// 从左到右扫描；先查“差多少”，再把当前元素登记，避免同一位置重复使用。

#include <cassert>       // assert：核对找到的下标与无解情况。
#include <cstddef>       // std::size_t：安全表示 vector 的下标。
#include <iostream>      // std::cout：打印示范答案。
#include <optional>      // std::optional：有答案时含一对下标，无解时为空。
#include <unordered_map> // std::unordered_map：记录以前见过的值与下标。
#include <utility>       // std::pair、std::make_pair：把两个下标作为结果。
#include <vector>        // std::vector：保存输入数组。

using Answer = std::optional<std::pair<std::size_t, std::size_t>>;

Answer two_sum(const std::vector<int>& values, int target) {
    // long long 保存值，避免做 target - 当前值时在 int 范围两端溢出。
    std::unordered_map<long long, std::size_t> seen;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const long long needed = static_cast<long long>(target) - values[i];
        const auto hit = seen.find(needed); // 只在已经扫描过的下标中找。
        if (hit != seen.end()) {
            return std::make_pair(hit->second, i); // 下标必然不同。
        }
        // 未找到才登记当前项；emplace 对同一值保留较早的下标。
        seen.emplace(static_cast<long long>(values[i]), i);
    }
    return std::nullopt; // 没有任何一对符合要求。
}

int main() {
    const auto first = two_sum({2, 7, 11, 15}, 9);
    assert(first && first->first == 0 && first->second == 1);
    const auto repeated = two_sum({3, 3}, 6);
    assert(repeated && repeated->first == 0 && repeated->second == 1);
    const auto missing = two_sum({1, 2}, 10);
    assert(!missing);

    std::cout << "two-sum indexes: " << first->first << ' '
              << first->second << '\n';
    std::cout << "duplicates use two positions: " << repeated->first
              << ' ' << repeated->second << '\n';
}
```

**运行观察：** `[2,7,11,15]`、目标 9 返回 `0 1`；两个 3、目标 6 也返回 `0 1`。无解输入返回空结果。

> [!question]- 自测
> 1. 为什么计算 `target - values[i]`？
> 2. 为什么先查，再登记当前值？
> 3. `seen` 存的是“值→什么”？
> 4. `std::optional` 为空时能否直接用 `->first`？

> [!success]- 核对
> 要查的是组成目标所缺的值。先查限制另一项必须来自旧下标。表中值对应出现过的位置；optional 为空时不得读取其中的 pair。
