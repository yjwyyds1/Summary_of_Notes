// ============================================================================
// 02_perf_benchmark.cpp
// 目的：演示一个规范一点的微基准——对比“没有 reserve 的 push_back”与“先 reserve”。
// 前置：知道 vector 扩容会重新分配并搬移元素（10-20 已讲）。
// 程序步骤：
//   1. 预热一轮，避免把第一次的页错误算进结果；
//   2. 每种写法跑多轮，取最小值；
//   3. 把结果写进 volatile 变量，防止整个循环被优化掉；
//   4. 打印耗时与容量差异。
// 观察点：
//   - 不 reserve 时容量按倍数增长，会发生多次重新分配；
//   - 先 reserve 只分配一次；
//   - 倍率随元素类型、数量、编译器版本变化，不要背数字。
// 编译：g++ -std=c++17 -O2 -Wall -Wextra 02_perf_benchmark.cpp -o 02_perf_benchmark
// ============================================================================

#include <algorithm>   // std::min
#include <chrono>      // 计时
#include <cstddef>     // std::size_t
#include <iostream>    // std::cout
#include <vector>      // std::vector

namespace {

constexpr std::size_t kCount = 2'000'000;   // 元素个数
constexpr int kRounds = 5;                  // 每项测几轮

// volatile：告诉编译器“这个变量可能被外部改变”，不要把它优化掉
volatile long long g_sink = 0;

// 计时：跑 kRounds 轮，返回最小毫秒数
template <typename Fn>
double best_ms(Fn&& fn) {
    double best = 1e18;
    for (int round = 0; round < kRounds; ++round) {
        const auto start = std::chrono::steady_clock::now();
        fn();
        const auto end = std::chrono::steady_clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(end - start).count());
    }
    return best;
}

// 写法 A：不预留，靠 vector 自己扩容
std::size_t push_without_reserve() {
    std::vector<int> values;
    for (std::size_t i = 0; i < kCount; ++i) {
        values.push_back(static_cast<int>(i));
    }
    g_sink += static_cast<long long>(values.size());   // 防止整体被优化掉
    return values.capacity();
}

// 写法 B：先 reserve，再 push_back
std::size_t push_with_reserve() {
    std::vector<int> values;
    values.reserve(kCount);
    for (std::size_t i = 0; i < kCount; ++i) {
        values.push_back(static_cast<int>(i));
    }
    g_sink += static_cast<long long>(values.size());
    return values.capacity();
}

}  // namespace

int main() {
    std::cout << "元素数 " << kCount << "，每项取 " << kRounds << " 轮最小值\n";

    // 预热：让页表、分配器缓存先热起来
    (void)push_without_reserve();
    (void)push_with_reserve();

    std::size_t cap_a = 0;
    std::size_t cap_b = 0;
    const double ms_a = best_ms([&] { cap_a = push_without_reserve(); });
    const double ms_b = best_ms([&] { cap_b = push_with_reserve(); });

    std::cout << "\n=== 结果 ===\n";
    std::cout << "不 reserve：容量 " << cap_a << "，最小耗时 " << ms_a << " ms\n";
    std::cout << "先 reserve：容量 " << cap_b << "，最小耗时 " << ms_b << " ms\n";
    std::cout << "比值 不预留/预留 = " << ms_a / ms_b << "\n";
    std::cout << "\n注意：容量按实现的增长策略变化；耗时倍数随机器与优化级别变化\n";
    std::cout << "sink = " << g_sink << "（防止结果被优化掉）\n";
    return 0;
}
