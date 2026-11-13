---
date: 2026-10-31
question_id: OS-SIGNAL-001
standard: C++17 / Linux POSIX
---

# 03_signal · 把信号转成普通流程的停止请求

用途：用 `sigaction` 安装处理器，再用 `raise(SIGUSR1)` 通知自己。处理器只设置停止标记，输出与收尾全部交给普通执行流程。它不向其他进程发信号，不创建线程或子进程，也不需要额外输入。

先修：[[学习/2026-10-20/00-先读懂C++代码与对象|函数、指针与取地址]]、[[学习/2026-10-24/01-进程与线程|进程与线程]]。详解：[[学习/2026-10-31/03-信号与安全退出|信号与安全退出]] · [[学习/学习导航/2026-10-31-学习导航|当天导航]] · [[学习/2026-10-31/示例代码/03_signal.cpp|源码文件]]。

## 完整注释源码

下面代码与同目录 `03_signal.cpp` 逐字一致。

```cpp
// 目的：观察“信号处理器只记下请求，普通执行流程再处理请求”。
// 平台：Linux + C++17；使用 POSIX sigaction，不能直接当成纯标准 C++ 跨平台代码。
// 先修：函数、全局变量、if、for，以及 &obj 表示取得对象地址。
// 步骤：安装处理器 -> 确保 SIGUSR1 未被阻塞 -> 自己发信号 -> 主流程退出循环。
// 观察：全部输出都来自 main；处理器没有输出、分配内存或加锁。
// 程序不向其他进程发信号，不创建线程或子进程，不需要外部输入。

#include <csignal>   // std::sig_atomic_t、std::raise：信号标记类型与给自己发信号。
#include <cstdio>    // std::perror：仅在普通执行流程中打印系统调用错误。
#include <iostream>  // std::cout/std::cerr：普通流程的教学输出。
#include <signal.h>  // POSIX 的 sigaction、sigset_t、sigprocmask 等接口。

// 0 表示尚未请求停止，1 表示请求过停止。
// volatile sig_atomic_t 适合本例“同一线程的处理器与正常流程”之间的简单标记。
// 它不是 std::atomic，也不能据此推导 volatile 能解决多个线程的数据竞争。
volatile std::sig_atomic_t stop_requested = 0;

// extern "C" 指定 C 语言链接，匹配信号处理回调的接口形式。
// 参数 int 是收到的信号编号；本例只注册一种信号，不使用它，所以省略参数名。
// void 表示没有返回值；这里不调用库函数，只把停止标记置为 1。
extern "C" void on_signal(int) {
    stop_requested = 1;
}

int main() {
    // {} 先把结构体各成员初始化；struct 是为了明确这是类型而不是同名函数。
    struct sigaction action{};
    // 这里只保存函数入口地址，没有立即调用 on_signal。
    action.sa_handler = on_signal;
    action.sa_flags = 0;  // 使用最基本的处理方式，不启用额外行为标志。

    // sa_mask 是处理器执行期间额外阻塞的信号集合；用 API 初始化为空集合。
    if (::sigemptyset(&action.sa_mask) == -1) {
        std::perror("sigemptyset(action)");
        return 1;
    }
    // &action 让内核读取这份配置；nullptr 表示本例不保存原来的处理方式。
    if (::sigaction(SIGUSR1, &action, nullptr) == -1) {
        std::perror("sigaction");
        return 1;
    }

    // 信号屏蔽状态可从启动者继承。显式解除本例信号的阻塞，避免它一直待处理。
    // 必须先安装处理器，再解除阻塞；SIGUSR1 的默认动作是终止进程。
    sigset_t unblocked{};
    if (::sigemptyset(&unblocked) == -1) {
        std::perror("sigemptyset(unblocked)");
        return 1;
    }
    if (::sigaddset(&unblocked, SIGUSR1) == -1) {
        std::perror("sigaddset");
        return 1;
    }
    // 本例只有一个线程，所以使用 sigprocmask；多线程程序应另行设计信号接收。
    if (::sigprocmask(SIG_UNBLOCK, &unblocked, nullptr) == -1) {
        std::perror("sigprocmask");
        return 1;
    }

    // 最多执行五个模拟工作单元；第二个单元后给当前线程发送 SIGUSR1。
    for (int unit = 1; unit <= 5; ++unit) {
        std::cout << "完成工作单元 " << unit << '\n';
        if (unit == 2) {
            // 信号未被阻塞且已安装处理器；raise 在处理器返回后才返回。
            // raise 返回 0 表示成功，非 0 表示失败；不假定失败一定设置 errno。
            if (std::raise(SIGUSR1) != 0) {
                std::cerr << "raise 失败\n";
                return 1;
            }
        }
        if (stop_requested != 0) {
            // 此时已经回到 main，可以正常使用输出、RAII 与业务清理代码。
            std::cout << "主流程收到停止请求，不再开始下一个工作单元\n";
            break;  // 只退出 for，继续执行 main 后面的清理位置。
        }
    }

    std::cout << "主流程执行收尾并正常返回\n";
    return 0;  // 正常离开 main 的作用域；本例没有需要回收的线程或子进程。
}
```

## 编译与输出观察

在 Linux/WSL 中进入本目录：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic 03_signal.cpp -o 03_signal
./03_signal
```

系统调用成功且没有额外外部信号干扰时，预期输出：

```text
完成工作单元 1
完成工作单元 2
主流程收到停止请求，不再开始下一个工作单元
主流程执行收尾并正常返回
```

`SIGUSR1` 本来默认终止进程，本例先安装处理器，才解除阻塞和发送。`raise` 触发处理器且等它返回后继续，随后 `main` 读到 1，`break` 跳出循环，所以不输出工作单元 3。[sigaction(2)](https://man7.org/linux/man-pages/man2/sigaction.2.html)、[raise(3)](https://man7.org/linux/man-pages/man3/raise.3.html)

## 读码时要盯住的关系

```mermaid
flowchart LR
    A[main 安装处理器] --> B[raise 给自己发信号]
    B --> C[处理器写标记]
    C --> D[main 读标记]
    D --> E[跳出循环并收尾]
```

- `action` 保存“怎么处理”的配置，`action.sa_handler` 保存函数入口；安装时没有调用处理器。
- `unblocked` 保存要解除阻塞的信号集合；先安装处理器再解除阻塞，避免误用 `SIGUSR1` 默认动作。
- `stop_requested` 是处理器与本线程普通代码之间的简单通知。`volatile sig_atomic_t` 不是一般线程同步工具。
- 所有 `perror`、`cout` 和 `cerr` 都位于普通执行流程。处理器中只有赋值，便于核查异步信号安全。
- 最后的文字只是教学收尾位置。本例没有文件、工作线程或持久化业务，不把这行输出当作生产服务已经完成全部清理。

实际运行记录见 [[学习/2026-10-31/示例验证|当天示例验证]]。技术边界另见 [signal-safety(7)](https://man7.org/linux/man-pages/man7/signal-safety.7.html) 和 [C++17 草案 N4659，§4.7.1 [intro.races]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf)，访问日期 2026-10-31。

自测：为什么不能把 `std::cout` 搬到 `on_signal` 中？为什么把本例改成多线程后还需要重新设计停止请求的同步？
