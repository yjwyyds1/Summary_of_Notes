// C++17 学习示例：vector 与 list 都能保存一串整数，访问和插入代价不同。
// 关键条件：list 的常数时间插入，必须已经拿到目标位置的迭代器。

#include <algorithm> // std::equal：比较两种容器最终存的值。
#include <cassert>   // assert：检查元素和保留的 list 迭代器。
#include <iostream>  // std::cout：打印示范结果。
#include <iterator>  // std::next：从首项向前走一步。
#include <list>      // std::list：支持前后移动的序列容器。
#include <vector>    // std::vector：支持按下标快速访问的连续容器。

int main() {
    std::vector<int> contiguous{10, 20, 30};
    std::list<int> linked{10, 20, 30};

    // vector 的第 1 号下标直接访问第二个元素，复杂度为常数。
    assert(contiguous[1] == 20);
    // list 没有 operator[]，这里从 begin 向前走一步取得插入位置。
    auto second = std::next(linked.begin());
    auto first = linked.begin(); // 保留一个指向 10 的迭代器。
    assert(*second == 20);

    // 已知插入位置时，list 插入单个元素不搬动原有元素。
    linked.insert(second, 15);
    assert(*first == 10 && *second == 20); // 旧迭代器仍可指向原节点。

    // vector 在中间插入，需要为新元素腾出位置；可能需要重新分配。
    contiguous.insert(contiguous.begin() + 1, 15);
    // 从此重新获取位置；不要继续使用插入前缓存的 vector 迭代器。
    assert(contiguous[2] == 20);
    assert(std::equal(contiguous.begin(), contiguous.end(),
                      linked.begin(), linked.end()));

    std::cout << "both contain: ";
    for (int value : contiguous) std::cout << value << ' ';
    std::cout << '\n';
    // 查找 list 的第 k 个位置需要从已知位置逐步走过去；
    // 因此“插入动作 O(1)”不代表“按下标找位置再插入总是 O(1)”。
}
