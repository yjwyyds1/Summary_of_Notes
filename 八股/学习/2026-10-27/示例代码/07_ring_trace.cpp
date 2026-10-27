// 目的：单线程推演物理 4 格、可用 3 格的环形队列边界。
// 前置：本程序不含并发；并发内存序见 05_spsc_ring.cpp。
// 观察：前三次 push 成功，第四次失败；pop 1 后 push 4 成功。
#include <array>      // std::array：固定槽位。
#include <cstddef>    // std::size_t：数组位置。
#include <iostream>   // std::cout：打印操作结果。

int main() {
    std::array<int, 4> slots{};
    std::size_t head = 0, tail = 0;
    auto next = [](std::size_t i) { return (i + 1) % 4; };
    auto push = [&](int value) { // 按引用捕获数组和两个位置。
        const auto after = next(tail);
        if (after == head) return false; // 留一格，不能覆盖未读槽。
        slots[tail] = value; // 先写当前 tail 所指的槽。
        tail = after;        // 再移动到下一可写位置。
        return true;
    };
    auto pop = [&](int& value) {
        if (head == tail) return false; // 空时不读槽。
        value = slots[head];
        head = next(head);
        return true;
    };
    std::cout << push(1) << push(2) << push(3) << push(4) << '\n';
    int first = 0;
    if (pop(first)) std::cout << "popped=" << first << '\n';
    std::cout << "push4again=" << push(4)
              << " head=" << head << " tail=" << tail << '\n';
}
