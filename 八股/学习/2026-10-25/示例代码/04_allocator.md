# 04_allocator · STL 分配器与预留容量

← [[学习/学习导航/2026-10-25-学习导航|学习导航]]

**用途**：可在 Obsidian 中读完整注释源码；同名 `.cpp` 可独立编译。基线 C++17。

```cpp
// 目的：用计数分配器观察 vector 何时申请原始存储。
// 前置：allocator_traits 会调用符合分配器契约的 allocate/deallocate。
#include <cassert>  // assert：检查元素数量。
#include <cstddef>  // size_t：容量与字节数量。
#include <iostream> // cout：打印分配次数。
#include <memory>   // allocator：委托标准分配器实际取得存储。
#include <vector>   // vector：演示 reserve。

inline std::size_t allocations = 0; // 教学计数；仅在单线程 main 中使用。
template<class T> struct CountingAllocator {
    using value_type = T; // 容器要求的元素类型。
    CountingAllocator() = default;
    template<class U> CountingAllocator(const CountingAllocator<U>&) noexcept {}
    T* allocate(std::size_t n) { // n 是元素个数，不是字节数。
        ++allocations;
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { // 与取得的元素数量配对。
        std::allocator<T>{}.deallocate(p, n);
    }
};
template<class T, class U> bool operator==(CountingAllocator<T>, CountingAllocator<U>) { return true; }
template<class T, class U> bool operator!=(CountingAllocator<T>, CountingAllocator<U>) { return false; }

int main() {
    std::vector<int, CountingAllocator<int>> prices;
    prices.reserve(3); // 先申请足够容纳三个 int 的空间，但 size 仍是 0。
    assert(prices.empty());
    for (int p : {101, 102, 103}) prices.push_back(p); // 在已留容量内构造元素。
    assert(prices.size() == 3);
    std::cout << "allocations: " << allocations << '\n';
    // 只观察本例的 vector 存储申请，不计输出库或进程其他分配。
}
```

**观察**：核对断言与输出；涉及性能的示例仅演示语义，不以一次运行宣称速度。
