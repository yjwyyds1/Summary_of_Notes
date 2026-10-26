---
date: 2026-10-23
type: 学习详解
related_question: NET-EPOLL-001
cpp_standard: C++17 + Linux epoll
---

# 06 · select / poll / epoll：同时看多个连接何时可读

← [[学习/学习导航/2026-10-23-学习导航|今日导航]] · [[八股测试/2026-10-23#NET-EPOLL-001 · select、poll 和 epoll 怎么比较|对应自测]] · [[学习/2026-10-23/示例代码/06_epoll_lt_et|源码阅读版]]

> [!abstract] 面经来源
> [七牛云候选人 SEULL 的一面复盘](https://www.nowcoder.com/discuss/359091166204239872)记了为什么用 epoll、同步/异步和阻塞问题；[后台 C++ 十五面候选人复盘](https://www.nowcoder.com/discuss/353157613774905344)记了 select 与 epoll/poll 比较。下面的管道程序和表格自行设计，技术行为核对 Linux 手册。

## 文件描述符、就绪、多路复用

Linux 用**文件描述符 fd**（非负整数）代表打开的 socket、管道等资源。**可读就绪**大致表示现在读通常不会因“无数据可读”而等待；它不是“已经给你读完”或“保证读到完整业务消息”。**I/O 多路复用**让一个线程等多个 fd 中哪几个有事件，再针对这些 fd 调 `read/recv`。这仍需要应用自己读取与解析。[epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)。

| 接口 | 每次等待怎么提交关注列表 | 返回后怎么找就绪者 | 特点 |
| --- | --- | --- | --- |
| `select` | 传入 fd 集合，调用会改写集合，下次需重设 | 检查集合成员 | Linux glibc `fd_set` 的 `FD_SETSIZE` 常为 1024；别泛化成所有系统绝对限制 |
| `poll` | 传 `pollfd` 数组 | 检查数组里的 `revents` | 没有 `select` 同一 `fd_set` 固定范围；仍要扫描返回数组 |
| `epoll` | `epoll_ctl` 注册/修改关注，`epoll_wait` 等 | 返回发生的事件列表 | Linux 专有，关注集可长期留在内核；适合很多连接、少数就绪的情况 |

参考 [select(2)](https://man7.org/linux/man-pages/man2/select.2.html)、[poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html)、[epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)。**不能背“epoll 永远 O(1)、连接数无限、ET 一定更快”**：操作成本和性能取决于注册/修改频率、活跃度、系统限制和应用处理。Linux 手册描述其在大量监视 fd 时可良好扩展，没有给应用端到端恒定复杂度承诺。

```mermaid
flowchart LR
  F["很多 socket / fd"] --> E["epoll_ctl：建立关注集"]
  E --> W["epoll_wait：等就绪事件"]
  W --> R["应用 read/recv"]
  R --> P["应用协议解析器"]
```

## 从一个连接扩展到许多连接

只处理一个阻塞 socket 时，线程可以直接 `recv` 等它。若同一线程负责许多连接，在 A 上等数据时，B 即使已经有数据也可能迟迟得不到处理。多路复用先问内核“哪些 fd 现在可能可读”，再对返回的 fd 做真正的读。**通知不是数据本身**：接到可读事件后，仍要调用 `read/recv`，仍可能读到半条业务消息。

`select` 和 `poll` 的入门差别可以从“每轮怎样交名单”理解：调用者准备关注的 fd 集合或数组，等待返回后再查看谁就绪。`epoll` 将关注集合通过 `epoll_ctl` 注册到一个 epoll 对象，`epoll_wait` 从其中返回事件。想监控一个新连接，要注册；不再使用要正确移除和关闭。这个模型减少了反复传递、扫描大关注集合的某些开销，但不消除处理每个活跃连接的实际工作，也不保证在所有规模都更快。

每个连接还需要自己的**状态**：读到的半条消息放哪里、当前写缓冲剩多少、连接是否正在关闭。就绪事件只提醒“现在值得尝试”，不替你管理这些状态。事件循环若一直给一个忙连接读到天荒地老，别的连接可能被饿住；生产服务还要限制每轮处理量，并处理错误、半关闭和资源回收。

## LT 与 ET：什么时候会再次提醒

**LT 水平触发**是默认模式：只要读缓冲仍有可读数据，后续等待仍可能继续报告可读。**ET 边缘触发**在就绪状态变化时通知；若收到事件后只读一小部分，再去无限等待下一次边缘，可能长时间等不到提醒。Linux 手册建议 ET 搭配非阻塞 fd，并读/写到 `EAGAIN` 再等待。[epoll(7) Level-triggered and edge-triggered](https://man7.org/linux/man-pages/man7/epoll.7.html)。

## 代码：真实 Linux epoll，Windows 仅展示平台提示

本例用**管道**代替网络 socket，避免端口和外部服务。管道读端与 socket 一样可作为 epoll 监视对象；`pipe` 得到两个 fd，`Fd` 类析构时关闭 fd，避免忘记清理。Windows 没有 Linux epoll，所以该平台只编译另一提示分支。

```cpp
// C++17 / Linux 示例：用管道模拟可读 fd，观察 epoll ET 的读到 EAGAIN。
// Windows 分支仅说明平台限制，不声称已验证 Linux 分支。
#include <iostream> // cout/cerr：输出结果或错误。

#ifdef __linux__
#include <cerrno>       // errno、EAGAIN：识别“暂时读完”。
#include <fcntl.h>      // fcntl、O_NONBLOCK：设置非阻塞。
#include <stdexcept>    // runtime_error：系统调用失败时停止示例。
#include <sys/epoll.h>  // epoll_create1/ctl/wait 与事件类型。
#include <unistd.h>     // pipe、read、write、close。

class Fd {
    int value_; // Linux 文件描述符是非负整数；-1 表示无效。
public:
    explicit Fd(int value) : value_(value) {} // 获取这个描述符的关闭责任。
    ~Fd() { if (value_ >= 0) close(value_); } // 离开作用域自动关闭。
    Fd(const Fd&) = delete;             // 禁止复制，防止两个对象重复 close。
    Fd& operator=(const Fd&) = delete;
    int get() const { return value_; } // 借用数值给系统调用，不转移所有权。
};

int main() {
    int ends[2]; // ends[0] 读端，ends[1] 写端。
    if (pipe(ends) == -1) throw std::runtime_error("pipe failed");
    Fd reader(ends[0]);
    Fd writer(ends[1]);
    const int flags = fcntl(reader.get(), F_GETFL, 0); // 读取当前状态标志。
    if (flags == -1 || fcntl(reader.get(), F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl failed");
    Fd epoll_fd(epoll_create1(EPOLL_CLOEXEC));
    if (epoll_fd.get() == -1) throw std::runtime_error("epoll_create1 failed");
    epoll_event wanted{}; // 零初始化其它字段。
    wanted.events = EPOLLIN | EPOLLET; // 可读 + 边缘触发；去掉 EPOLLET 即 LT。
    wanted.data.fd = reader.get();     // 返回事件时定位是哪个 fd。
    if (epoll_ctl(epoll_fd.get(), EPOLL_CTL_ADD, reader.get(), &wanted) == -1)
        throw std::runtime_error("epoll_ctl failed");
    if (write(writer.get(), "AB", 2) != 2) throw std::runtime_error("write failed");
    epoll_event happened{};
    const int count = epoll_wait(epoll_fd.get(), &happened, 1, 1000);
    if (count != 1 || !(happened.events & EPOLLIN))
        throw std::runtime_error("expected readable event");
    char buffer[8];
    for (;;) { // ET 下读到暂时无数据，再等待下一次边缘。
        const auto n = read(reader.get(), buffer, sizeof buffer);
        if (n > 0) { std::cout.write(buffer, n); continue; }
        if (n == -1 && errno == EINTR) continue; // 信号打断，本次操作重试。
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (n == 0) break; // 管道写端关闭，读到 EOF。
        throw std::runtime_error("read failed");
    }
    std::cout << " drained to EAGAIN\n";
}
#else
int main() {
    std::cout << "This example requires Linux epoll; read the annotated Linux branch.\n";
}
#endif

```

沿着程序追：`pipe` 创建读/写端；`fcntl` 把读端设非阻塞；`epoll_create1` 创建事件对象；`epoll_ctl` 注册读端及 `EPOLLIN|EPOLLET`；写端写入 `AB` 后 `epoll_wait` 返回；读取循环直到 `EAGAIN`。`errno==EINTR` 是被信号打断时重试，`read==0` 是写端关闭后的 EOF。真实网络服务还要处理多连接、公平性、异常事件、发送缓冲与半关闭。

**口述**：三者都等待多个 fd 的就绪。select/poll 每轮把关注列表交给调用，应用要扫描结果；epoll 将关注集持久注册，等待返回就绪事件。LT 没读完仍可再提醒，ET 要非阻塞并尽量读到 `EAGAIN`。不能把就绪通知说成异步替应用读完数据。

> [!question]- 小变式：只有两个长期忙碌的连接，epoll 一定比 poll 快吗？
> 不能这么说。连接数、活跃比例、注册变动与平台实现都会影响成本，少量连接用 poll 也可能足够；应在目标负载下测量。
