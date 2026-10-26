// 目的：展示两个独立原子计数器与对齐隔离的布局；不把时长当性能结论。
// 前置：atomic 的 relaxed 只保证这里的原子计数，不发布其他业务状态。
#include <atomic>   // atomic：各线程独立更新各自计数器。
#include <cassert>  // assert：校验最终数值。
#include <iostream> // cout：输出布局信息。
#include <functional> // ref：把原子对象按引用传给线程。
#include <new>      // hardware_destructive_interference_size：C++17 实现提供的间距。
#include <thread>   // thread：两个工作者。

struct Adjacent { std::atomic<int> a{0}; std::atomic<int> b{0}; };
// alignas 让两个成员的起点至少按实现给出的间距对齐。
struct Separated {
    alignas(std::hardware_destructive_interference_size) std::atomic<int> a{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<int> b{0};
};

int main() {
    Adjacent near;
    Separated far;
    auto count = [](std::atomic<int>& x) { // 引用明确指向各自的原子对象。
        for (int i = 0; i < 1000; ++i) x.fetch_add(1, std::memory_order_relaxed);
    };
    std::thread first(count, std::ref(far.a)); // 两线程改不同变量。
    std::thread second(count, std::ref(far.b));
    first.join(); second.join(); // 读取前等待写入完成。
    assert(far.a.load() == 1000 && far.b.load() == 1000);
    std::cout << "near bytes: " << sizeof(near) << ", far bytes: " << sizeof(far) << '\n';
}
