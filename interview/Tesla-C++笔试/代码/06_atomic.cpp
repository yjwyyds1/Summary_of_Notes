#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

int main() {
    std::atomic_int active{0};
    std::atomic_int completed{0};
    const auto work = [&] {
        active.fetch_add(1, std::memory_order_relaxed);
        for (int i = 0; i < 1000; ++i)
            completed.fetch_add(1, std::memory_order_relaxed);
        active.fetch_sub(1, std::memory_order_relaxed);
    };
    std::thread first(work);
    std::thread second(work);
    first.join();
    second.join();
    assert(active.load() == 0 && completed.load() == 2000);
    completed.store(0); // Reset only after both writers finish.
    assert(completed.load() == 0);
    std::cout << "atomic OK\n";
}
