---
date: 2026-10-22
cpp_standard: C++17
type: 代码阅读版
source: 01_quicksort.cpp
---

# 01_quicksort · 末元素基准的快排

← [[学习/学习导航/2026-10-22-学习导航|学习导航]] · [[学习/2026-10-22/01-快速排序与复杂度|逐步讲解]] · [[学习/2026-10-22/示例代码/01_quicksort.cpp|可编译源码]]

先看“一个区间怎样分成两边”，再看递归；`id` 用于检查相等 key 的原先顺序。完整代码与同名 cpp 一致。

```cpp
// C++17 学习示例：自己写一版“末元素为基准”的快速排序。
// 看点：分区、递归、最坏比较次数，以及相同关键字的先后顺序。
// 这段代码用于理解算法，不把它当作 std::sort 的内部实现。

#include <algorithm> // std::is_sorted：检查最终是否按关键字有序。
#include <cassert>   // assert：验证例子的结果；正式输入校验不能只靠它。
#include <iostream>  // std::cout：打印观察结果。
#include <utility>   // std::swap：交换两个完整记录。
#include <vector>    // std::vector：保存一组连续的记录。

struct Record {
    int key; // 排序只比较 key，例如报价的价格或任务的优先级。
    char id; // 用 A/B/C 标识原始记录，方便观察稳定性。
};

// [left,right] 是包含两端的下标区间；返回基准最终所在下标。
int partition(std::vector<Record>& items, int left, int right,
              int& comparisons) {
    const int pivot = items[right].key; // 固定选最后一项的 key 作基准。
    int boundary = left;                // 下一项“小于等于基准”的安放位置。

    // scan 逐个查看基准之前的项；right 自己暂时留在末尾。
    for (int scan = left; scan < right; ++scan) {
        ++comparisons; // 统计一次 key 与基准的比较，不统计交换成本。
        if (items[scan].key <= pivot) {
            std::swap(items[boundary], items[scan]);
            ++boundary;
        }
    }
    std::swap(items[boundary], items[right]); // 基准落到左右两区中间。
    return boundary;
}

void quick_sort(std::vector<Record>& items, int left, int right,
                int& comparisons) {
    if (left >= right) return; // 0 或 1 项已经有序，也是递归停止条件。
    const int middle = partition(items, left, right, comparisons);
    quick_sort(items, left, middle - 1, comparisons);  // 左边继续分区。
    quick_sort(items, middle + 1, right, comparisons); // 右边继续分区。
}

int main() {
    // A、B 的 key 都是 2，排序前 A 在 B 前面。
    std::vector<Record> items{{2, 'A'}, {2, 'B'}, {1, 'C'}};
    int comparisons = 0;
    quick_sort(items, 0, static_cast<int>(items.size()) - 1, comparisons);

    // 比较器只看 key；id 是记录身份，不参与本次排序标准。
    assert(std::is_sorted(items.begin(), items.end(),
                          [](const Record& a, const Record& b) {
                              return a.key < b.key;
                          }));
    // 这版实现把原顺序 A、B 变成 B、A：它不保证稳定。
    assert(items[0].id == 'C' && items[1].id == 'B' && items[2].id == 'A');
    std::cout << "sorted ids: ";
    for (const Record& item : items) std::cout << item.id << ' ';
    std::cout << "\ncomparisons on 3 records: " << comparisons << '\n';

    // 最后元素总被选为基准时，已经有序的 5 项让分区极不均衡。
    std::vector<Record> sorted{{1, 'a'}, {2, 'b'}, {3, 'c'},
                               {4, 'd'}, {5, 'e'}};
    int worst_case_comparisons = 0;
    quick_sort(sorted, 0, static_cast<int>(sorted.size()) - 1,
               worst_case_comparisons);
    assert(worst_case_comparisons == 4 + 3 + 2 + 1);
    std::cout << "sorted-input comparisons: "
              << worst_case_comparisons << '\n';
}
```

**运行观察：** 小例子排序后是 `C B A`；5 个已经升序的元素让这版末元素基准快排执行 `10` 次 key 比较。这是该程序的计数，不代表所有快排或 `std::sort` 的输出。

> [!question]- 遮住代码复述
> 1. `partition` 的 boundary 在指向什么？
> 2. 为什么扫描不到最后一项？
> 3. 左右递归为什么不能包含基准本身？
> 4. `2A,2B,1C` 的 A/B 相对顺序是否保留？

> [!success]- 核对
> boundary 是下一项小于等于基准的放置位置。末项暂作基准，最后再换到位。基准已到正确位置，递归只处理两侧。此实现给出 C B A，A/B 被反转。
