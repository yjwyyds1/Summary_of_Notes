// 目的：用 compare_exchange_weak 维护单个原子的最大值。
// 前提：只维护整数本身，不借它发布别的非原子对象。
// 观察：候选 17、42、30、8 中最终最大值为 42。
#include <atomic>     // std::atomic<int>、CAS、relaxed。
#include <iostream>   // std::cout。
#include <thread>     // std::thread、join。

void update_max(std::atomic<int>& peak, int candidate) {
    int observed = peak.load(std::memory_order_relaxed); // 本次看到的旧值。
    while (observed < candidate &&
           !peak.compare_exchange_weak(observed, candidate,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed)) {
        // 失败时 observed 已被写成当前值；weak 也可能虚假失败。
        // 回到条件重新判断；不需要在循环体里手工 load。
    }
}

int main() {
    std::atomic<int> peak{0};
    std::thread a([&] { update_max(peak, 17); update_max(peak, 42); });
    std::thread b([&] { update_max(peak, 30); update_max(peak, 8); });
    a.join(); b.join();
    std::cout << "peak=" << peak.load(std::memory_order_relaxed) << '\n';
}
