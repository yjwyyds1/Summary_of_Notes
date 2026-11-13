// 目的：按“字节”计算最长无重复连续子串；不把 UTF-8 字节等同字符。
// 前置：输入为 std::string；此函数对所有 256 种字节值都能索引。
// 步骤：右边界逐字节前进；若重复字节在当前窗口内，左边界移到其后。
#include <algorithm> // std::max：保证左边界不会后退。
#include <array>     // 固定 256 项，保存每种字节最近出现的位置。
#include <cassert>   // assert：覆盖空串和 abba 边界。
#include <string>    // std::string：字节序列。

std::size_t longest_unique_bytes(const std::string& s) { // const&：只读借用。
    std::array<std::size_t, 256> last; // 下标对应 unsigned char 的 0..255。
    last.fill(s.size());              // 合法位置只到 size-1，size 代表尚未出现。
    std::size_t left = 0, best = 0;
    for (std::size_t right = 0; right < s.size(); ++right) {
        const auto byte = static_cast<unsigned char>(s[right]); // 避免 char 为负时越界。
        if (last[byte] != s.size()) left = std::max(left, last[byte] + 1);
        best = std::max(best, right - left + 1); // 当前窗口保证没有重复字节。
        last[byte] = right; // 用 size_t 保存位置，与 string::size() 的类型一致。
    }
    return best;
}

int main() {
    assert(longest_unique_bytes("") == 0);
    assert(longest_unique_bytes("abcabcbb") == 3);
    assert(longest_unique_bytes("abba") == 2); // left 不能从 2 倒退到 1。
}
