# 05_lru · LRU 缓存为何常用链表加哈希表

← [[学习/学习导航/2026-10-25-学习导航|学习导航]]

**用途**：可在 Obsidian 中读完整注释源码；同名 `.cpp` 可独立编译。基线 C++17。

```cpp
// 目的：实现容量固定的最小 LRU，演示查找、刷新和淘汰。
// 前置：list 迭代器与 unordered_map；本例只在单线程使用。
#include <cassert>       // assert：检查淘汰顺序。
#include <iostream>      // cout：打印结果。
#include <list>          // list：维护最近使用次序。
#include <optional>      // optional：区分未命中与合法的整数值。
#include <unordered_map> // unordered_map：按 key 找链表节点。
#include <utility>       // pair：存 key/value。

class LRU {
    using Entry = std::pair<int, int>;
    std::size_t limit_; // 最大条目数。
    std::list<Entry> order_; // 前端最近使用，尾端最久未使用。
    std::unordered_map<int, std::list<Entry>::iterator> index_; // key 到节点位置。
public:
    explicit LRU(std::size_t limit) : limit_(limit) {}
    std::optional<int> get(int key) {
        auto found = index_.find(key); // 平均常数时间查找；最坏不保证。
        if (found == index_.end()) return std::nullopt;
        order_.splice(order_.begin(), order_, found->second); // 原节点移到前端。
        return found->second->second; // 迭代器仍指向原节点，返回 value。
    }
    void put(int key, int value) {
        if (limit_ == 0) return; // 零容量永远不保存。
        auto found = index_.find(key);
        if (found != index_.end()) { // 更新已有项而不增加 size。
            found->second->second = value;
            order_.splice(order_.begin(), order_, found->second);
            return;
        }
        order_.emplace_front(key, value); // 新节点成为最近使用。
        try { index_.emplace(key, order_.begin()); }
        catch (...) { order_.pop_front(); throw; } // 表插入失败时撤销新链表节点。
        if (index_.size() > limit_) { // 只有成功插入后才可能超容量。
            index_.erase(order_.back().first);
            order_.pop_back();
        }
    }
};

int main() {
    LRU cache(2);
    cache.put(1, 10); cache.put(2, 20);
    assert(cache.get(1) == 10); // 1 变成最近使用。
    cache.put(3, 30); // 淘汰 2。
    assert(!cache.get(2));
    std::cout << "latest 1: " << *cache.get(1) << '\n';
}
```

**观察**：核对断言与输出；涉及性能的示例仅演示语义，不以一次运行宣称速度。
