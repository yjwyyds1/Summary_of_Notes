// ============================================================================
// 02_lru.cpp
// 目的：手写一个 O(1) 的 LRU 缓存，并把它该测的边界都测一遍。
// 前置：知道 list 的 splice、unordered_map 的迭代器稳定性（见当天 01、02 号专题）。
// 程序步骤：
//   1. 用 std::list 维护“最近使用 → 最久未用”的顺序，头部最新；
//   2. 用 unordered_map<key, list 迭代器> 做到 O(1) 定位；
//   3. get：命中则把结点 splice 到头部；未命中返回 -1；
//   4. put：已存在则更新并前移；不存在则插入头部，超容量删尾部；
//   5. 用经典容量 2 的序列验证淘汰行为，再补三个边界用例。
// 观察点：
//   - list::splice 只移动结点、不复制元素，迭代器保持有效，这是 O(1) 的关键；
//   - 淘汰时必须同时删除 list 中的结点和 map 中的条目，否则 map 留下悬垂迭代器；
//   - 容量为 0 时 put 不应插入任何元素。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 02_lru.cpp -o 02_lru
// ============================================================================

#include <iostream>        // std::cout
#include <list>            // std::list：双向链表，splice 为 O(1)
#include <optional>        // std::optional：表示“可能没有值”
#include <string>          // std::string：用例名称
#include <unordered_map>   // 哈希表：key → 链表迭代器

// ---------------------------------------------------------------------------
// LRU 缓存
// ---------------------------------------------------------------------------
class LRUCache {
public:
    explicit LRUCache(std::size_t capacity) : capacity_(capacity) {}

    // 读取：命中返回 value，并把该结点移到“最近使用”端
    std::optional<int> get(int key) {
        auto found = index_.find(key);
        if (found == index_.end()) {
            return std::nullopt;                 // 未命中
        }
        // splice 把结点从原位置摘下来插到 begin() 之前：O(1)，迭代器仍有效
        items_.splice(items_.begin(), items_, found->second);
        return found->second->value;
    }

    // 写入：存在则更新值并前移；不存在则插入，必要时淘汰最久未用
    void put(int key, int value) {
        if (capacity_ == 0) {
            return;                              // 容量为 0：什么都不存
        }
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->value = value;        // 更新已有键
            items_.splice(items_.begin(), items_, found->second);
            return;
        }
        if (items_.size() == capacity_) {
            // 淘汰尾部：先拿到 key，再同时清理链表与索引
            const int victim_key = items_.back().key;
            index_.erase(victim_key);
            items_.pop_back();
        }
        items_.push_front(Entry{key, value});
        index_[key] = items_.begin();
    }

    // 便于测试：按“最近使用 → 最久未用”的顺序导出键
    std::string order() const {
        std::string text;
        for (const Entry& entry : items_) {
            if (!text.empty()) text += " → ";
            text += std::to_string(entry.key);
        }
        return text.empty() ? "(空)" : text;
    }

    std::size_t size() const { return items_.size(); }

private:
    struct Entry {
        int key = 0;
        int value = 0;
    };

    std::size_t capacity_ = 0;
    std::list<Entry> items_;                                   // 头部最新
    std::unordered_map<int, std::list<Entry>::iterator> index_; // key → 结点
};

// 简单断言工具：失败时打印并返回 false
static bool expect(bool condition, const std::string& what) {
    std::cout << (condition ? "  [通过] " : "  [失败] ") << what << "\n";
    return condition;
}

int main() {
    std::cout << "=== 用例 1：容量 2 的经典序列 ===\n";
    LRUCache cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    std::cout << "顺序：" << cache.order() << "\n";
    expect(cache.get(1).value_or(-1) == 1, "get(1) 命中，返回 1");
    std::cout << "访问 1 之后顺序：" << cache.order() << "\n";
    cache.put(3, 3);   // 淘汰最久未用的 2
    std::cout << "插入 3 之后顺序：" << cache.order() << "\n";
    expect(!cache.get(2).has_value(), "get(2) 已被淘汰");
    cache.put(4, 4);   // 淘汰 1
    expect(!cache.get(1).has_value(), "get(1) 已被淘汰");
    expect(cache.get(3).value_or(-1) == 3, "get(3) 命中");
    expect(cache.get(4).value_or(-1) == 4, "get(4) 命中");

    std::cout << "\n=== 用例 2：更新已有键不应增加容量 ===\n";
    LRUCache small(2);
    small.put(1, 10);
    small.put(1, 20);
    expect(small.size() == 1, "同一个键写入两次，size 仍为 1");
    expect(small.get(1).value_or(-1) == 20, "值被更新为 20");

    std::cout << "\n=== 用例 3：容量为 0 ===\n";
    LRUCache empty(0);
    empty.put(1, 1);
    expect(empty.size() == 0, "容量 0 时不保存任何元素");

    std::cout << "\n=== 用例 4：访问顺序影响淘汰 ===\n";
    LRUCache order_test(2);
    order_test.put(1, 1);
    order_test.put(2, 2);
    (void)order_test.get(1);     // 1 变成最近使用
    order_test.put(3, 3);        // 应淘汰 2 而不是 1
    expect(!order_test.get(2).has_value(), "2 被淘汰");
    expect(order_test.get(1).value_or(-1) == 1, "1 仍在");
    return 0;
}
