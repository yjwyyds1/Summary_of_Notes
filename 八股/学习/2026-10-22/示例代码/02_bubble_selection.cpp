// C++17 学习示例：用同一组带身份的记录对照冒泡、选择与稳定排序。
// 重点看“比较次数”和“相同 key 的 A/B 是否仍按原先顺序出现”。

#include <algorithm> // std::stable_sort：标准库提供的稳定排序。
#include <cassert>   // assert：对照这组小样本的预期结果。
#include <iostream>  // std::cout：打印记录身份顺序。
#include <utility>   // std::swap：交换两条记录。
#include <vector>    // std::vector：保存待排序的记录。

struct Record {
    int key; // 只按这个整数排序。
    char id; // 原始身份；A、B 都为 key=2。
};

// 冒泡：发现左项更大才交换，等值时不交换，因此此实现保持稳定。
int bubble_sort(std::vector<Record>& data) {
    int comparisons = 0;
    for (std::size_t pass = 0; pass + 1 < data.size(); ++pass) {
        bool swapped = false; // 一整趟都没有交换，说明已经有序。
        for (std::size_t i = 0; i + 1 < data.size() - pass; ++i) {
            ++comparisons;
            if (data[i].key > data[i + 1].key) {
                std::swap(data[i], data[i + 1]);
                swapped = true;
            }
        }
        if (!swapped) break; // 已有序时，优化版冒泡可以提前停止。
    }
    return comparisons;
}

// 选择：每轮找剩余项中最小者，再与本轮开头交换。
int selection_sort(std::vector<Record>& data) {
    int comparisons = 0;
    for (std::size_t start = 0; start < data.size(); ++start) {
        std::size_t minimum = start;
        for (std::size_t i = start + 1; i < data.size(); ++i) {
            ++comparisons;
            if (data[i].key < data[minimum].key) minimum = i;
        }
        if (minimum != start) std::swap(data[start], data[minimum]);
    }
    return comparisons;
}

int main() {
    const std::vector<Record> input{{2, 'A'}, {2, 'B'}, {1, 'C'}};
    auto bubble = input;    // 各算法都从相同原始数据开始。
    auto selection = input; // auto 在这里推导出 std::vector<Record>。
    auto stable = input;

    const int bubble_checks = bubble_sort(bubble);
    const int selection_checks = selection_sort(selection);
    std::stable_sort(stable.begin(), stable.end(),
                     [](const Record& a, const Record& b) {
                         return a.key < b.key; // 只比较 key。
                     });

    assert(bubble[0].id == 'C' && bubble[1].id == 'A' && bubble[2].id == 'B');
    assert(selection[0].id == 'C' && selection[1].id == 'B'
           && selection[2].id == 'A');
    assert(stable[1].id == 'A' && stable[2].id == 'B');

    std::cout << "bubble: ";
    for (const auto& item : bubble) std::cout << item.id << ' ';
    std::cout << "checks=" << bubble_checks << '\n';
    std::cout << "selection: ";
    for (const auto& item : selection) std::cout << item.id << ' ';
    std::cout << "checks=" << selection_checks << '\n';
}
