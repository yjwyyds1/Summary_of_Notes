// 目的：用确定性测试序列检查 LRU 的容量、命中刷新、更新与淘汰。
// 前置：此练习用 list 保存键，模拟顺序；实现完整缓存参见 05_lru.cpp。
#include <algorithm> // find：教学版线性查找。
#include <cassert>   // assert：逐步核对期望。
#include <iostream>  // cout：报告测试结果。
#include <list>      // list：前端为最近使用。

int main() {
    std::list<int> recent; // 这里只演示状态转移，不把线性查找称为 O(1)。
    auto touch = [&](int key) { // 引用捕获，操作 main 中的同一条链表。
        auto found = std::find(recent.begin(), recent.end(), key);
        if (found != recent.end()) recent.erase(found); // 旧位置删除。
        recent.push_front(key); // 新位置为最近使用。
        if (recent.size() > 2) recent.pop_back(); // 超容量淘汰最久未用。
    };
    touch(1); touch(2);       // 顺序：2,1。
    touch(1);                 // 顺序：1,2；命中刷新。
    touch(3);                 // 顺序：3,1；应淘汰 2。
    assert(recent.front() == 3 && recent.back() == 1);
    assert(std::find(recent.begin(), recent.end(), 2) == recent.end());
    std::cout << "LRU sequence checked\n";
}
