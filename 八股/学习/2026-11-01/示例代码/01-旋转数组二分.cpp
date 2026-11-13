// 目的：在互不相同的旋转有序数组中找最小值，演示闭区间二分。
// 前置：原数组严格递增，旋转后仍非空；此算法不直接处理重复元素。
// 步骤：比较中点和右端，缩小答案所在区间；观察每轮区间都变短。
#include <cassert>    // assert：检查本地示例是否得到预期答案。
#include <stdexcept>  // invalid_argument：清楚报告不符合前提的输入。
#include <vector>     // vector：存放按下标访问的数组。

int rotated_min(const std::vector<int>& a) { // const&：只读借用数组，不复制。
    if (a.empty()) throw std::invalid_argument("empty array");
    std::size_t left = 0;             // left/right 是仍可能有最小值的闭区间两端。
    std::size_t right = a.size() - 1; // 空数组已在上方排除，所以减一安全。
    while (left < right) {            // 两端重合时答案已确定。
        const std::size_t mid = left + (right - left) / 2; // 避免 left+right 溢出。
        if (a[mid] > a[right]) left = mid + 1; // 最小值必在 mid 右边。
        else right = mid;                      // mid 自身仍可能是最小值。
    }
    return a[left]; // 循环不变量保证它是原数组的最小值。
}

int main() {
    assert(rotated_min({4, 5, 1, 2, 3}) == 1);
    assert(rotated_min({1, 2, 3}) == 1); // 未旋转也成立。
    assert(rotated_min({7}) == 7);       // 单元素无需循环。
}
