# 04_context_switch · Obsidian 完整源码阅读版

用途与解释：[[学习/2026-10-31/04-上下文切换]]。以下代码与同名 .cpp 完整一致。

```cpp
// 目的：读取 Linux 的上下文切换计数，观察主动等待前后的变化。
// 平台：Linux + C++17；getrusage 及两个切换计数字段属于这里使用的平台接口。
// 先修：结构体成员、取地址 &、const，以及毫秒表示时间长度。
// 步骤：读累计计数 -> 短暂等待三次 -> 再读计数 -> 输出两次读取的差值。
// 观察：自愿切换通常增加，非自愿切换可以为 0；具体数字不能事先写死。
// 程序只使用主线程，不创建子进程、不制造大量负载，正常运行约几十毫秒。

#include <chrono>        // std::chrono::milliseconds：带“毫秒”单位的时间长度。
#include <cstdio>        // std::perror：打印 getrusage 失败时的系统错误。
#include <iostream>      // std::cout：测量结束后再打印，减少输出对计数的干扰。
#include <thread>        // std::this_thread::sleep_for：让当前线程等待，不创建新线程。
#include <sys/resource.h> // Linux getrusage、rusage、RUSAGE_SELF 的声明。

int main() {
    // before/after 各保存一份内核累计资源统计；{} 使成员有已知初值。
    struct rusage before{};
    struct rusage after{};

    // RUSAGE_SELF 统计本进程所有线程之和；本例只有主线程，解释范围较简单。
    // &before 把对象地址交给函数，让它填写结果；返回 -1 表示失败。
    if (::getrusage(RUSAGE_SELF, &before) == -1) {
        std::perror("getrusage(before)");
        return 1;
    }

    // constexpr 表示编译时就能确定的常量；只等待三次，避免变成压力测试。
    constexpr int rounds = 3;
    constexpr int wait_ms = 20;
    for (int round = 0; round < rounds; ++round) {
        // sleep_for 让当前线程阻塞等待，CPU 可以执行其他可运行线程或空闲任务。
        // 实际恢复执行还要等到调度机会，因此不能承诺恰好在 20ms 时回来。
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    }

    if (::getrusage(RUSAGE_SELF, &after) == -1) {
        std::perror("getrusage(after)");
        return 1;
    }

    // ru_nvcsw 是自愿切换累计次数，ru_nivcsw 是非自愿切换累计次数。
    // 字段类型为 long；const 表示下面的输出不能意外修改这两个差值。
    const long voluntary = after.ru_nvcsw - before.ru_nvcsw;
    const long involuntary = after.ru_nivcsw - before.ru_nivcsw;

    // 此处已经拿到 after；下面的输出即使发生阻塞，也不计入刚才的测量区间。
    std::cout << "等待次数 = " << rounds << ", 每次请求 " << wait_ms << "ms\n";
    std::cout << "自愿上下文切换增量 = " << voluntary << '\n';
    std::cout << "非自愿上下文切换增量 = " << involuntary << '\n';
    std::cout << "这些是次数，不是一次切换的耗时，也不是系统调用次数\n";
    return 0;  // 没有额外线程或子进程，程序自然结束。
}
```

阅读时先按注释跟踪每一步，再回专题看场景与边界。验证见 [[学习/2026-10-31/示例验证]]。
