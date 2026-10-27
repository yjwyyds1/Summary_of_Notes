// 目的：安全模拟 ABA 的“值回来了，版本却变了”。
// 限制：只操作整数；不分配或释放无锁链表节点，不模拟 UAF。
// 观察：只比较值可成功；比较版本和值会拒绝旧观察。
#include <atomic>     // std::atomic 与 compare_exchange_strong。
#include <cstdint>    // uint32_t、uint64_t：明确打包宽度。
#include <iostream>   // std::cout。

std::uint64_t pack(std::uint32_t version, std::uint32_t value) {
    return (static_cast<std::uint64_t>(version) << 32) | value;
}

int main() {
    std::atomic<int> plain{1};
    int old_plain = plain.load(); // 模拟线程甲记住 A=1。
    plain.store(2);               // 模拟其他操作改成 B。
    plain.store(1);               // 又回到同一个整数 A。
    const bool plain_ok = plain.compare_exchange_strong(old_plain, 3);

    std::atomic<std::uint64_t> tagged{pack(0, 1)};
    auto old_tagged = tagged.load(); // 线程甲记住 (版本0, A)。
    tagged.store(pack(1, 2));        // B，版本递增。
    tagged.store(pack(2, 1));        // 值回 A，但版本再次递增。
    const bool tagged_ok = tagged.compare_exchange_strong(old_tagged, pack(1, 3));
    std::cout << "plain=" << plain_ok << " tagged=" << tagged_ok << '\n';
}
