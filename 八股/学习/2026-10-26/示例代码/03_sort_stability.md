---
date: 2026-10-26
type: 代码阅读版
source: 03_sort_stability.cpp
---

# 03_sort_stability：std::sort 与 std::stable_sort

← [[学习/2026-10-26/06-排序算法与std-sort|对应专题]] · [[学习/学习导航/2026-10-26-学习导航|今日导航]]

## 这份代码在做什么

1. 用 10 条记录（key 只有 1/2/3）比较两种排序；
2. 把规模放大到 200 条、只有 5 种 key，再比较一次；
3. 检查“相同 key 的 id 是否递增”，以此判断稳定性。

## 完整源码（与 `.cpp` 逐字一致）

```cpp
// ============================================================================
// 03_sort_stability.cpp
// 目的：用同一批含重复 key 的记录，对比 std::sort 与 std::stable_sort 的结果。
// 前置：知道“稳定”指相等元素的相对顺序保持不变（当天 06 号专题）。
// 程序步骤：
//   1. 构造 10 条记录，key 只有 1/2/3 三种，id 表示原始到达顺序；
//   2. 用 std::sort 按 key 升序，打印结果；
//   3. 用 std::stable_sort 排同一份数据，打印结果；
//   4. 检查每组相同 key 的 id 是否递增。
// 观察点：
//   - stable_sort 保证相等 key 的相对顺序不变，输出一定递增；
//   - std::sort 不保证稳定，本次输出是否变化取决于实现与数据，不能据此下普遍结论。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 03_sort_stability.cpp -o 03_sort_stability
// ============================================================================

#include <algorithm>   // std::sort、std::stable_sort
#include <iostream>    // std::cout
#include <string>      // std::string
#include <vector>      // std::vector

// 一条记录：key 参与排序，id 表示原始到达顺序
struct Record {
    int key = 0;
    int id = 0;
};

// 按 key 升序；key 相等时返回 false，表示“两者等价”
static bool by_key(const Record& lhs, const Record& rhs) {
    return lhs.key < rhs.key;
}

// 打印记录序列
static void print(const std::string& title, const std::vector<Record>& records) {
    std::cout << title << "\n  ";
    for (const Record& record : records) {
        std::cout << "(" << record.key << ",id=" << record.id << ") ";
    }
    std::cout << "\n";
}

// 检查相同 key 的 id 是否递增（即相对顺序是否保持）
static bool is_stable(const std::vector<Record>& records) {
    for (std::size_t i = 1; i < records.size(); ++i) {
        if (records[i].key == records[i - 1].key && records[i].id < records[i - 1].id) {
            return false;
        }
    }
    return true;
}

int main() {
    // 打乱到达顺序：key 与实际 id 顺序不一致，才能看出稳定性差异
    const std::vector<Record> original = {
        {2, 0}, {1, 1}, {3, 2}, {2, 3}, {1, 4}, {3, 5}, {2, 6}, {1, 7}, {3, 8}, {2, 9}
    };
    print("原始顺序：", original);

    std::vector<Record> sorted = original;
    std::sort(sorted.begin(), sorted.end(), by_key);
    print("\nstd::sort 结果：", sorted);
    std::cout << "  相同 key 的 id 是否保持递增：" << (is_stable(sorted) ? "是" : "否") << "\n";

    std::vector<Record> stable_sorted = original;
    std::stable_sort(stable_sorted.begin(), stable_sorted.end(), by_key);
    print("\nstd::stable_sort 结果：", stable_sorted);
    std::cout << "  相同 key 的 id 是否保持递增：" << (is_stable(stable_sorted) ? "是" : "否") << "\n";

    // ------------------------------------------------------------------
    // 规模放大：小数组常常走插入排序，看起来“稳定”；大数组才可能暴露
    // ------------------------------------------------------------------
    std::vector<Record> large;
    large.reserve(200);
    for (int i = 0; i < 200; ++i) {
        large.push_back(Record{i % 5, i});   // 只有 5 种 key，大量相等元素
    }

    std::vector<Record> large_sorted = large;
    std::sort(large_sorted.begin(), large_sorted.end(), by_key);
    std::vector<Record> large_stable;
    large_stable = large;
    std::stable_sort(large_stable.begin(), large_stable.end(), by_key);

    std::cout << "\n=== 规模放大到 200 条（只有 5 种 key）===\n";
    std::cout << "std::sort       稳定？" << (is_stable(large_sorted) ? "是" : "否") << "\n";
    std::cout << "std::stable_sort 稳定？" << (is_stable(large_stable) ? "是" : "否") << "\n";

    std::cout << "\n说明：stable_sort 的“是”是标准保证；std::sort 的结果只是本次实现的观察，"
                 "标准只说不保证稳定，不承诺一定打乱。\n";
    return 0;
}
```

## 本机运行结果（GCC 13.1，x86-64，`-O0`；为节省篇幅省略了“原始顺序”一行）

```text
=== 10 条记录 ===
std::sort 结果：
  (1,id=1) (1,id=4) (1,id=7) (2,id=0) (2,id=3) (2,id=6) (2,id=9) (3,id=2) (3,id=5) (3,id=8)
  相同 key 的 id 是否保持递增：是
std::stable_sort 结果：
  (1,id=1) (1,id=4) (1,id=7) (2,id=0) (2,id=3) (2,id=6) (2,id=9) (3,id=2) (3,id=5) (3,id=8)
  相同 key 的 id 是否保持递增：是

=== 规模放大到 200 条（只有 5 种 key）===
std::sort       稳定？否
std::stable_sort 稳定？是
```

## 怎么读结果

- 10 条时 `std::sort` 看起来也稳定：小数组常走插入排序，碰巧保持了顺序。**这正说明不能靠一次实验判断“实现稳不稳定”。**
- 200 条时 `std::sort` 出现了顺序变化，`std::stable_sort` 始终稳定。前者是本次实现的观察，后者是标准保证。
- 结论：有稳定性需求就用 `std::stable_sort`，不要赌 `std::sort` 的运气。

## 对应测试题

[[八股测试/2026-10-26#STL-SORT-001 · 排序与 std::sort]]
