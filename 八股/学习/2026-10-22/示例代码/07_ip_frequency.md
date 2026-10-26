---
date: 2026-10-22
cpp_standard: C++17
type: 代码阅读版
source: 07_ip_frequency.cpp
---

# 07_ip_frequency · 按键计数和并列规则

← [[学习/学习导航/2026-10-22-学习导航|学习导航]] · [[学习/2026-10-22/07-练习-IP频次|练习详解]] · [[学习/2026-10-22/示例代码/07_ip_frequency.cpp|可编译源码]]

最好先做 [[八股测试/2026-10-22#EX-20261022-001 · 找出出现次数最多的 IP|测试题]]。本页就是详细注释答案，完整代码与同名 cpp 一致。

```cpp
// C++17 综合小练习：找出出现次数最多的 IP 字符串。
// 若最高次数并列，取字典序较小的 IP；这样不依赖哈希表遍历顺序。

#include <cassert>       // assert：验证次数与并列处理。
#include <iostream>      // std::cout：打印答案。
#include <optional>      // std::optional：空输入时没有获胜项。
#include <string>        // std::string：保存 IP 的文本形式。
#include <unordered_map> // std::unordered_map：按 IP 累计次数。
#include <utility>       // std::pair：保存结果的 IP 和出现次数。
#include <vector>        // std::vector：输入的一串 IP。

using Result = std::optional<std::pair<std::string, int>>;

Result most_frequent(const std::vector<std::string>& ips) {
    if (ips.empty()) return std::nullopt; // 不凭空造一个 IP。

    std::unordered_map<std::string, int> counts;
    for (const std::string& ip : ips) {
        ++counts[ip]; // 首次访问缺失键时先插入 0，再自增。
    }

    std::string best_ip;
    int best_count = 0;
    for (const auto& item : counts) {
        const std::string& ip = item.first; // 键：IP 字符串。
        const int count = item.second;      // 值：出现次数。
        if (count > best_count ||
            (count == best_count && ip < best_ip)) {
            best_ip = ip;
            best_count = count;
        }
    }
    return std::make_pair(best_ip, best_count);
}

int main() {
    const std::vector<std::string> ips{
        "10.0.0.1", "10.0.0.2", "10.0.0.1", "10.0.0.3", "10.0.0.2"};
    const Result answer = most_frequent(ips);
    assert(answer && answer->first == "10.0.0.1" && answer->second == 2);
    assert(!most_frequent({}));
    std::cout << answer->first << " appears " << answer->second
              << " times\n";
}
```

**运行观察：** `10.0.0.1 appears 2 times`。另一个 IP 也出现 2 次，程序依训练版的字典序约定选择较小的那个；空输入返回无结果。

> [!question]- 自测
> 1. 首次执行 `++counts[ip]`，原来不存在的键怎么处理？
> 2. 为什么并列比较不能靠 unordered_map 迭代顺序？
> 3. 空输入返回什么？
> 4. 哪些因素会让“整体 O(n)”的简单说法需要补条件？

> [!success]- 核对
> `operator[]` 建立值初始化的计数后自增；unordered_map 无固定顺序，所以显式按字典序比较。空输入为 nullopt，字符串长度和哈希冲突会影响时间成本。
