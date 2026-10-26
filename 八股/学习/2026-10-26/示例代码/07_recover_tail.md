---
date: 2026-10-26
type: 示例代码阅读版
id: EX-20261026-001
---

# 07_recover_tail · 练习：遇到不完整日志尾部怎样恢复

用途：如果长度为 4 但只剩 2 字节，直接按 4 读取可能越界；如果把残片当新记录，恢复后的订单状态会被污染。

先读：[[学习/2026-10-26/07-练习-截断日志尾部|专题详解]]。

## 与 cpp 逐字一致的完整源码

```cpp
// 目的：从“长度 1 字节 + payload”教学格式中只恢复完整记录。
// 前置：理解下标从 0 开始、vector::size 和 string(begin,end)。
// 观察：第二条声明 4 字节但只有 DE 两字节，因此只恢复 ABC。
#include <cstddef>      // size_t
#include <cstdint>      // uint8_t
#include <iostream>     // cout
#include <string>       // string
#include <vector>       // vector

std::vector<std::string> recover(const std::vector<std::uint8_t>& log) {
    std::vector<std::string> records;
    std::size_t position = 0; // 下一条记录长度字段的位置。
    while (position < log.size()) { // 至少还有一个长度字节。
        const std::size_t length = log[position++]; // 读取后移动到 payload 起点。
        if (length > log.size() - position) break; // 先检验，再访问；右侧无溢出。
        const auto begin = log.begin() + static_cast<std::ptrdiff_t>(position);
        const auto end = begin + static_cast<std::ptrdiff_t>(length);
        records.emplace_back(begin, end); // 只把完整 payload 加入结果。
        position += length; // 指向下一条的长度字节。
    }
    return records; // 返回值优化/移动由编译器与语言规则处理。
}

int main() {
    const std::vector<std::uint8_t> log = {3, 'A', 'B', 'C', 4, 'D', 'E'};
    for (const auto& record : recover(log)) // const & 避免复制每条字符串。
        std::cout << record << '\n';
}
```

## 读完后核对

- 预期只打印一行 `ABC`。这是解析测试，不是崩溃注入或校验和测试。
- `position` 指向下一条记录的长度字节。
- 先取 `length`，然后用 `length > log.size() - position` 判断尾部是否不足。
- 够长才构造 `std::string` 并移动 position；不足立即停止。
- 测试同时覆盖完整首条、残缺尾条和空输入。

- 自测变式：把第二个长度改成 2，预测输出；再把首字节改成 250，解释为什么必须有最大长度和剩余量检查。
