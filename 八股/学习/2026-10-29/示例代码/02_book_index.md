# 订单簿价格层、ID 索引与撤单 · 注释源码阅读版

先修：[[学习/2026-10-29/02-订单簿索引与撤单|专题详解]]。以下与保存的 cpp 和专题代码块逐字一致。

```cpp
// 目的：展示“有序价格层 + 同价队列 + ID 索引”的教学版买盘。
// 前置：只维护买侧，不做撮合；生产实现需另做并发和容量设计。
// 步骤：加三单、连续撤两单，观察最优买价何时改变。
#include <functional>    // std::greater 让买价从高到低排序。
#include <iostream>      // 打印检查结果。
#include <list>          // 同价订单保持先入先出的顺序。
#include <map>           // 有序价格层，begin 为最优买价。
#include <unordered_map> // ID 到订单位置的平均常数时间索引。

struct Order { int id; int leaves; }; // 同价层内仍保留逐订单 ID 与数量。

class BidBook {
    using Queue = std::list<Order>; // 一个价格层对应一条 FIFO 队列。
    std::map<int, Queue, std::greater<int>> levels_; // 高价在前。
    struct Location { int price; Queue::iterator it; }; // 撤单定位信息。
    std::unordered_map<int, Location> by_id_; // 不能覆盖活跃订单 ID。
public:
    bool add(int id, int price, int quantity) {
        if (quantity <= 0 || price <= 0 || by_id_.count(id)) return false;
        Queue& queue = levels_[price]; // 缺少价层时创建一个空队列。
        queue.push_back({id, quantity}); // 新订单排在同价队尾。
        auto it = queue.end(); --it;     // 指向刚插入的节点。
        by_id_.emplace(id, Location{price, it}); // 建立反向索引。
        return true;
    }
    bool cancel(int id) {
        auto found = by_id_.find(id);
        if (found == by_id_.end()) return false; // 未知 ID 不能乱扣数量。
        auto level = levels_.find(found->second.price); // 找所属价格层。
        level->second.erase(found->second.it); // list 擦除仅使该节点迭代器失效。
        by_id_.erase(found);              // 不能留下指向已删除节点的索引。
        if (level->second.empty()) levels_.erase(level); // 空层必须去掉。
        return true;
    }
    int best_bid() const { return levels_.empty() ? 0 : levels_.begin()->first; }
};

int main() {
    BidBook book;
    book.add(7, 100, 2); book.add(8, 100, 3); book.add(9, 99, 1);
    std::cout << book.best_bid() << std::endl; // 100。
    book.cancel(7); std::cout << book.best_bid() << std::endl; // 仍为 100。
    book.cancel(8); std::cout << book.best_bid() << std::endl; // 变为 99。
}
```

## 读后自检

价 100 层最后一个订单撤掉后，为什么要删价层？同一个 ID 再次 add 应允许覆盖吗？

这份源码只证明教学模型中的步骤；生产规则和边界见专题详解。
