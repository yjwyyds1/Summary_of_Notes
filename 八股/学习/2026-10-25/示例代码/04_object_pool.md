---
date: 2026-10-25
type: 代码阅读版
source: 04_object_pool.cpp
---

# 04_object_pool：把 new 拆成“分配 + 构造”

← [[学习/2026-10-25/04-内存池与分配器|对应专题]] · [[学习/2026-10-25/07-练习-固定大小对象池|练习详解]] · [[学习/学习导航/2026-10-25-学习导航|今日导航]]

## 这份代码在做什么

1. 用 `alignas(T) std::byte storage_[N][sizeof(T)]` 预留原始存储；
2. 启动时把所有块挂到空闲链表；
3. `acquire()` 摘一块，用 placement new 构造；
4. `release()` 显式析构再挂回；
5. 观察池满、地址复用、借出计数。

## 完整源码（与 `.cpp` 逐字一致）

```cpp
// ============================================================================
// 02_object_pool.cpp
// 目的：手写一个“固定大小对象池”，把 new 拆成“分配 + 构造”两步看清楚。
// 前置：读过 new/delete 与 placement new（当天 01 号专题），知道对象析构要显式调用。
// 程序步骤：
//   1. 池子启动时预留 BlockSize 块原始存储，全部挂到空闲链表；
//   2. acquire()：摘一块，用 placement new 构造；
//   3. release()：显式析构，再把存储挂回空闲表；
//   4. 观察地址复用、池满时的行为、以及“构造/析构次数”与“分配次数”的区别。
// 观察点：
//   - 池子满时返回 nullptr（也可以选择抛异常，取决于接口契约）；
//   - release 后再次 acquire，拿到的地址是被复用的旧地址；
//   - 全程没有 new/delete 参与对象存储，只有 placement new 与显式析构。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 02_object_pool.cpp -o 02_object_pool
// ============================================================================

#include <cstddef>    // std::byte、std::size_t
#include <iostream>   // std::cout
#include <new>        // placement new
#include <utility>    // std::forward：把参数原样转发给 T 的构造函数
#include <vector>     // std::vector：用于空闲链表（教学中优先可读性）

// 被池化的小对象：模拟一笔订单（字段少，便于观察构造/析构）
struct Order {
    long id = 0;
    double price = 0.0;
    int qty = 0;

    Order() { std::cout << "    Order 构造 @" << this << "\n"; }
    explicit Order(long order_id) : id(order_id), price(100.0), qty(1) {
        std::cout << "    Order 构造 @" << this << " id=" << id << "\n";
    }
    ~Order() { std::cout << "    Order 析构 @" << this << "\n"; }
};

// ---------------------------------------------------------------------------
// 固定大小对象池
// ---------------------------------------------------------------------------
template <typename T, std::size_t BlockCount>
class ObjectPool {
public:
    ObjectPool() {
        // 启动时把每块存储都挂到空闲表：此时还没有任何 T 对象存在
        for (std::size_t i = 0; i < BlockCount; ++i) {
            free_slots_.push_back(&storage_[i]);
        }
    }

    // 池子不复制：复制会让两块池子争用同一批地址
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ~ObjectPool() = default;   // 池子里若还有存活对象，其析构由调用方负责

    // 取得一块存储并构造 T；池满返回 nullptr
    T* acquire() {
        if (free_slots_.empty()) {
            return nullptr;
        }
        void* slot = free_slots_.back();
        free_slots_.pop_back();
        ++outstanding_;
        return new (slot) T();   // placement new：只在已有存储上构造
    }

    // 带参数构造：把参数原样转发给 T 的构造函数
    template <typename... Args>
    T* acquire(Args&&... args) {
        if (free_slots_.empty()) {
            return nullptr;
        }
        void* slot = free_slots_.back();
        free_slots_.pop_back();
        ++outstanding_;
        return new (slot) T(std::forward<Args>(args)...);
    }

    // 释放：先析构对象，再把存储挂回空闲表
    void release(T* object) {
        if (object == nullptr) {
            return;
        }
        object->~T();                        // 显式调用析构函数
        free_slots_.push_back(object);       // 地址重新可用
        --outstanding_;
    }

    std::size_t available() const { return free_slots_.size(); }
    std::size_t outstanding() const { return outstanding_; }
    static constexpr std::size_t capacity() { return BlockCount; }

private:
    // 原始存储：alignas(T) 保证每块满足 T 的对齐要求；这里不能写成 T storage_[N]
    alignas(T) std::byte storage_[BlockCount][sizeof(T)];
    std::vector<void*> free_slots_;   // 空闲块地址
    std::size_t outstanding_ = 0;     // 已借出、尚未归还的数量
};

int main() {
    std::cout << "=== 1. 构造池子（此时没有 Order 对象）===\n";
    ObjectPool<Order, 3> pool;
    std::cout << "容量 " << ObjectPool<Order, 3>::capacity()
              << "，可用 " << pool.available() << "\n";

    std::cout << "\n=== 2. 借出三个对象（观察构造）===\n";
    Order* a = pool.acquire(1001);
    Order* b = pool.acquire(1002);
    Order* c = pool.acquire();
    std::cout << "已借出 " << pool.outstanding() << "，可用 " << pool.available() << "\n";

    std::cout << "\n=== 3. 池满时的行为 ===\n";
    Order* d = pool.acquire(1004);
    std::cout << "第四次 acquire 返回 " << (d == nullptr ? "nullptr（池满）" : "有效指针") << "\n";

    std::cout << "\n=== 4. 归还后地址被复用 ===\n";
    std::cout << "归还 b（地址 " << b << "）\n";
    pool.release(b);
    Order* reused = pool.acquire(2002);
    std::cout << "再次借出的地址 " << reused << "，与原地址"
              << (reused == b ? "相同（复用）" : "不同") << "\n";

    std::cout << "\n=== 5. 归还全部，观察析构 ===\n";
    pool.release(a);
    pool.release(c);
    pool.release(reused);
    std::cout << "可用 " << pool.available() << "，已借出 " << pool.outstanding() << "\n";

    std::cout << "\n说明：池子析构时不会替未归还的对象调用析构，所有权契约由调用方遵守\n";
    return 0;
}
```

## 本机运行结果（GCC 13.1，x86-64，`-O0`）

```text
=== 1. 构造池子（此时没有 Order 对象）===
容量 3，可用 3

=== 2. 借出三个对象（观察构造）===
    Order 构造 @0x31a27ff710 id=1001
    Order 构造 @0x31a27ff6f8 id=1002
    Order 构造 @0x31a27ff6e0
已借出 3，可用 0

=== 3. 池满时的行为 ===
第四次 acquire 返回 nullptr（池满）

=== 4. 归还后地址被复用 ===
归还 b（地址 0x31a27ff6f8）
    Order 析构 @0x31a27ff6f8
    Order 构造 @0x31a27ff6f8 id=2002
再次借出的地址 0x31a27ff6f8，与原地址相同（复用）

=== 5. 归还全部，观察析构 ===
    Order 析构 @0x31a27ff710
    Order 析构 @0x31a27ff6e0
    Order 析构 @0x31a27ff6f8
可用 3，已借出 0
```

## 怎么读结果

- 第 1 段没有任何 `Order 构造` 输出，说明 `storage_` 只是原始存储，没有提前构造对象。
- 第 4 段：归还的地址和再次借出的地址相同，这就是“复用”，也是池子相比 `malloc` 省下的部分。
- 第 5 段：析构顺序按归还顺序，因为空闲链表是后进先出。

## 对应测试题

[[八股测试/2026-10-25#EX-20261025-001 · 固定大小对象池]]
