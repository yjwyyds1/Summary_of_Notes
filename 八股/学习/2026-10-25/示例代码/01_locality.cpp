// 目的：观察连续 vector 与链式 list 的元素地址和遍历结果；不是性能跑分。
// 前置：理解引用、迭代器和 range-for；编译标准 C++17。
#include <cassert>  // assert：检查两种容器计算出同一结果。
#include <iostream> // cout：打印结果。
#include <list>     // list：节点式序列，节点地址不保证连续。
#include <vector>   // vector：普通元素连续存放。

int main() {
    std::vector<int> prices{101, 102, 103}; // 三个相邻的 int 元素。
    std::list<int> nodes{101, 102, 103};   // 逻辑顺序相同，节点布局未指定。
    int vector_sum = 0;
    int list_sum = 0;
    for (int price : prices) vector_sum += price; // 顺序读取连续元素。
    for (int price : nodes) list_sum += price;    // 每一步经迭代器前进。
    assert(vector_sum == list_sum); // 功能等价不表示性能等价。
    std::cout << vector_sum << '\n';
    std::cout << "vector element gap: "
              << (&prices[1] - &prices[0]) << '\n'; // 同一 vector 内相邻元素相差一个元素位置。
    // 不对 list 节点地址做性能推断：一次打印地址不能代表缓存行为。
}
