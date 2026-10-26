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
