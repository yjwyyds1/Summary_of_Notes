---
date: 2026-10-31
id: OS-SYSCALL-001
type: 源码阅读版
---
# 01_syscall — 普通计算与系统调用

← [[学习/学习导航/2026-10-31-学习导航]] · [[学习/2026-10-31/01-用户态与系统调用|专题详解]] · [同名 CPP](01_syscall.cpp)

用途：在 Linux C++17 中实际执行 `SYS_getpid`、`SYS_write`，比较普通计算与内核服务，并正确处理短写、EINTR 和 EBADF。先修：[[学习/2026-10-24/01-进程与线程|进程的身份与资源]]。

下方是与同名 CPP 逐字一致的完整注释源码。编译运行命令及结果范围见 [[学习/2026-10-31/示例验证]]；未读取该验证记录前，不把预期输出当作已经运行的结果。

```cpp
// Linux + C++17 教学示例：把普通计算、进入内核和系统调用失败分开观察。
// 前置：认识函数、指针、数组长度；只在 Linux 编译，无 Windows 替代分支。
// 步骤：普通加法 -> SYS_getpid -> SYS_write 输出 -> 非法 fd 返回 EBADF。
// 观察：syscall() 仍是 libc 函数，但它会通过体系结构规定的入口请求内核。
// 本例没有创建子进程、文件或线程；正常终端下立即结束，不用于测量性能。
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE // glibc：使 syscall() 的声明可见，必须放在头文件之前。
#endif
#include <cerrno>       // errno、EINTR、EBADF、EIO：失败原因的符号名称。
#include <cstddef>      // std::size_t：表示字节数的无符号整数类型。
#include <cstdio>       // printf、perror：显示结果与诊断信息。
#include <sys/syscall.h> // SYS_getpid、SYS_write：系统调用号，避免手写架构相关数字。
#include <unistd.h>     // syscall、STDOUT_FILENO：通用调用入口与标准输出 fd。

int add(int a, int b) {
    // 纯粹对两个整数计算；这个操作本身不需要操作系统提供特权服务。
    // 编译器可能把它内联，所以不能把源码函数数目当成机器 CALL 指令数目。
    return a + b;
}

// 将 data 指向的 count 个字节交给内核，全部写完返回 true。
// const char*：可以移动指针，但不能通过它修改指向的字符。
bool write_all(int fd, const char* data, std::size_t count) {
    while (count > 0) {
        // syscall 的结果类型是 long；SYS_write 的成功结果是本次写入的字节数。
        // 即使只写一行，也按“可能短写”的接口规则处理，不能假定一次写完。
        const long written = ::syscall(SYS_write, fd, data, count);
        if (written == -1) {
            if (errno == EINTR) {
                // 该次调用在写入字节前被信号打断：重试尚未写出的同一段。
                continue;
            }
            return false; // 其它错误交给调用者报告，不无限重试。
        }
        if (written == 0) {
            // 对本例非空输出，零进展无法推进循环；转为明确失败避免忙转。
            errno = EIO;
            return false;
        }
        // written 已经确认是正数，才转换为无符号字节数，避免把 -1 变成大整数。
        const auto done = static_cast<std::size_t>(written);
        data += done;  // 下一次从尚未写出的字符开始。
        count -= done; // 只写剩余字节，避免把已经写出的前缀重复输出。
    }
    return true;
}

int main() {
    const int sum = add(20, 22); // 普通用户态计算；sum 应为 42。
    const long pid = ::syscall(SYS_getpid); // 明确发起 getpid 系统调用。
    if (pid == -1) {
        std::perror("syscall(SYS_getpid)");
        return 1;
    }

    // 字符数组末尾还有字符串结束符 '\0'；输出时减 1，不把结束符写到终端。
    const char message[] = "hello from SYS_write\n";
    if (!write_all(STDOUT_FILENO, message, sizeof(message) - 1)) {
        std::perror("syscall(SYS_write)");
        return 1;
    }

    // fd 是内核为本进程管理的打开资源编号；-1 不是有效的文件描述符。
    // 指针和长度仍然有效，因此这里只演示可预期的错误，不做越界或非法解引用。
    errno = 0;
    const long result = ::syscall(SYS_write, -1, message, sizeof(message) - 1);
    const int saved_error = errno; // 失败后立即保存，后面的库调用可能改变 errno。
    if (result != -1 || saved_error != EBADF) {
        std::fprintf(stderr, "unexpected result=%ld errno=%d\n", result, saved_error);
        return 1;
    }

    // pid 每次运行可能变化；这不是性能测试，也不能据此推导切换耗时。
    std::printf("ordinary add = %d\n", sum);
    std::printf("kernel returned pid = %ld\n", pid);
    std::printf("invalid fd: result = -1, errno = EBADF\n");
    return 0;
}
```

## 输出与阅读检查

普通计算应得到 42；PID 随运行变化；非法 fd 的分支预期验证 -1 与 EBADF。`data += done` 跳过已写出的字节，`count -= done` 只保留剩余数量。

请解释：为什么检查 -1 后才把结果转换为 `std::size_t`？为什么 errno 要在输出诊断前保存？为什么看到 PID 没变不能证明执行期间没有任务切换？完整解释在 [[学习/2026-10-31/01-用户态与系统调用|专题详解]]。

技术依据：[syscall(2)](https://man7.org/linux/man-pages/man2/syscall.2.html)、[write(2)](https://man7.org/linux/man-pages/man2/write.2.html)、[errno(3)](https://man7.org/linux/man-pages/man3/errno.3.html)，访问于 2026-10-31。
