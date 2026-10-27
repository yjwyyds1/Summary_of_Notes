---
date: 2026-10-27
type: 示例代码阅读版
id: CON-ABA-001
---

# 06_aba_tag · ABA 与节点内存回收为什么棘手

先读：[[学习/2026-10-27/06-ABA与内存回收|专题详解]]。

## 与 cpp 逐字一致的完整注释源码

```cpp
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
```

## 看完后核对

正常运行预期 `plain=1 tagged=0`。顺序模拟只证明比较值的行为，不证明真正无锁算法安全。

- `plain` 依次经历 1→2→1，旧的 expected=1 仍能 CAS 成功。
- `pack(version,value)` 将两段 32 位值组合成一个原子 64 位整数。
- `tagged` 回到 value=1 时 version 已是 2，旧期望 `(0,1)` 不再匹配。
- 本代码没有分配/释放动态节点，因此可以安全运行；回收仅在正文说明。

**变式：**状态 `(版本0,A)` 经 `(版本1,B)` 回到 `(版本2,A)`；分别写出只比较 A 和比较整个二元组时 CAS 结果。
