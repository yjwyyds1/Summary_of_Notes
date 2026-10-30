---
date: 2026-10-26
type: 代码阅读版
source: 01_topk.cpp
---

# 01_topk：三种 TopK 做法放在同一批数据上

← [[学习/2026-10-26/05-优先级队列与TopK|对应专题]] · [[学习/学习导航/2026-10-26-学习导航|今日导航]]

## 这份代码在做什么

1. 用固定种子生成 100 万个随机整数；
2. 分别用“全排序”“大小 k 的小根堆”“`nth_element`”取前 100；
3. 校验三者结果一致，并打印耗时。

## 完整源码（与 `.cpp` 逐字一致）

```cpp
// ============================================================================
// 01_topk.cpp
// 目的：把“求最大的 k 个”的三种做法放在同一批数据上比较。
// 前置：知道 priority_queue 默认是大根堆、小根堆要传 greater（当天 05 号专题）。
// 程序步骤：
//   1. 生成 100 万个随机整数（固定种子，结果可复现）；
//   2. 方法 A：全排序后取前 k；
//   3. 方法 B：维护大小为 k 的小根堆；
//   4. 方法 C：nth_element 划分后取前 k；
//   5. 校验三种方法结果一致，并打印耗时。
// 观察点：
//   - 结果一致：三种方法都应得到同一批“最大的 k 个数”；
//   - 耗时差异随 k 变化：k 小的时候堆更有优势，k 接近 n 时差距缩小；
//   - 计时只作相对参考，不要背绝对数字。
// 编译：g++ -std=c++17 -O2 01_topk.cpp -o 01_topk
// ============================================================================

#include <algorithm>   // std::sort、std::nth_element、std::greater
#include <chrono>      // 计时
#include <functional>  // std::greater
#include <iostream>    // std::cout
#include <numeric>     // std::iota（此处未用，保留说明可扩展）
#include <queue>       // std::priority_queue
#include <random>      // 随机数生成
#include <vector>      // std::vector

namespace {

constexpr std::size_t kDataSize = 1'000'000;   // 数据量
constexpr std::size_t kTopK = 100;             // 取前多少个

// 计时：返回毫秒
template <typename Fn>
double time_ms(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// 方法 A：全排序后取前 k（降序）
std::vector<int> topk_by_full_sort(std::vector<int> data, std::size_t k) {
    std::sort(data.begin(), data.end(), std::greater<int>());
    data.resize(k);
    return data;
}

// 方法 B：大小为 k 的小根堆
std::vector<int> topk_by_heap(const std::vector<int>& data, std::size_t k) {
    std::priority_queue<int, std::vector<int>, std::greater<int>> heap;   // 小根堆
    for (int value : data) {
        if (heap.size() < k) {
            heap.push(value);
        } else if (value > heap.top()) {
            heap.pop();
            heap.push(value);
        }
    }
    std::vector<int> result;
    result.reserve(heap.size());
    while (!heap.empty()) {
        result.push_back(heap.top());
        heap.pop();
    }
    std::sort(result.begin(), result.end(), std::greater<int>());   // 只为便于比较
    return result;
}

// 方法 C：nth_element 划分（注意会修改传入副本）
std::vector<int> topk_by_nth_element(std::vector<int> data, std::size_t k) {
    std::nth_element(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(k),
                     data.end(), std::greater<int>());
    data.resize(k);
    std::sort(data.begin(), data.end(), std::greater<int>());       // 只为便于比较
    return data;
}

}  // namespace

int main() {
    // 固定种子：同样的数据、可复现的结果
    std::mt19937 generator(20261026);
    std::uniform_int_distribution<int> distribution(0, 10'000'000);
    std::vector<int> data(kDataSize);
    for (int& value : data) {
        value = distribution(generator);
    }

    std::cout << "数据量 " << kDataSize << "，取前 " << kTopK << " 个\n";

    // 三种方法各跑一次，记录耗时
    std::vector<int> result_sort;
    std::vector<int> result_heap;
    std::vector<int> result_nth;
    double ms_sort = 0.0;
    double ms_heap = 0.0;
    double ms_nth = 0.0;

    ms_sort = time_ms([&] { result_sort = topk_by_full_sort(data, kTopK); });
    ms_heap = time_ms([&] { result_heap = topk_by_heap(data, kTopK); });
    ms_nth = time_ms([&] { result_nth = topk_by_nth_element(data, kTopK); });

    std::cout << "\n=== 结果一致性 ===\n";
    std::cout << "全排序 与 小根堆 相同：" << (result_sort == result_heap ? "是" : "否") << "\n";
    std::cout << "全排序 与 nth_element 相同：" << (result_sort == result_nth ? "是" : "否") << "\n";

    std::cout << "\n=== 耗时（毫秒，单次观察）===\n";
    std::cout << "全排序取前 k     ：" << ms_sort << " ms\n";
    std::cout << "小根堆           ：" << ms_heap << " ms\n";
    std::cout << "nth_element      ：" << ms_nth << " ms\n";

    std::cout << "\n=== 前 10 个最大值（校验用）===\n";
    for (std::size_t i = 0; i < 10 && i < result_sort.size(); ++i) {
        std::cout << result_sort[i] << (i + 1 == 10 ? "\n" : " ");
    }
    std::cout << "提示：k 很小时堆更省；k 接近 n 或只做一次时，nth_element/全排序可能更划算\n";
    return 0;
}
```

## 本机运行结果（GCC 13.1，x86-64，`-O2`）

```text
数据量 1000000，取前 100 个

=== 结果一致性 ===
全排序 与 小根堆 相同：是
全排序 与 nth_element 相同：是

=== 耗时（毫秒，单次观察）===
全排序取前 k     ：71.0435 ms
小根堆           ：0.5483 ms
nth_element      ：4.7233 ms

=== 前 10 个最大值（校验用）===
9999987 9999986 9999979 9999973 9999968 9999935 9999926 9999916 9999912 9999889
```

## 怎么读结果

- 三者结果一致，说明三种方法都对；差异只在成本。
- 这里 k=100 远小于 n=100 万，小根堆只维护 100 个元素，优势明显；`nth_element` 要分区整段数据，介于两者之间。
- 如果 k 接近 n，全排序的劣势会缩小甚至消失；如果数据是流式的、无法重复访问，只有堆可行。
- 单次计时受系统负载影响，结论看量级与趋势。

## 对应测试题

[[八股测试/2026-10-26#STL-HEAP-001 · TopK]]
