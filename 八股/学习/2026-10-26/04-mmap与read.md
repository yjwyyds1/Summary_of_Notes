---
date: 2026-10-26
type: 学习专题
cpp_standard: C++17
id: IO-MMAP-001
learning_status: 待学习
---

# mmap 与 read：按访问方式选接口

← [[学习/学习导航/2026-10-26-学习导航|今日导航]] · [[八股测试/2026-10-26|今日自测]]

> [!abstract] 今天要弄懂什么
> 用 mmap 读取一个大型索引文件，比 read 总是更快吗？映射被截断的文件可能发生什么？

## 先把名词说清楚

`mmap` 可以理解为把文件的一段‘接到’进程的虚拟地址区：代码读数组下标，内核在缺页时准备相应页面。`read` 则要求你准备一个缓冲区，并明确调用搬入数据。映射是地址关系，不等于所有文件字节马上装入 RAM。

## 为什么会遇到这个问题

订单簿快照、索引和历史行情回放访问方式不同：顺序扫、随机跳、是否共享、是否会被改写。选接口时应看这些条件，而不是背‘零拷贝一定快’。

## 用一个具体过程理解

读一个 4 KB 片段时，`read` 会把内核取得的内容复制到你提供的 `char[]`，你明确知道调用何时发生；`mmap` 则先建立地址关系，随后第一次碰某个页可能触发缺页，由内核准备该页。因此 mmap 可能减少显式复制和系统调用，但把部分成本放到了访问时刻。若热路径突然访问冷页，尾延迟可能抖动。`MAP_PRIVATE` 适合把文件当输入并允许进程内私有修改，修改走写时复制，不更新原文件；`MAP_SHARED` 可用于共享文件页，但其他进程可见和断电持久是两件事，还需要同步协议与 `msync`/文件同步判断。映射区有自己的生命周期，成功建立后关闭 fd 不会自动解除映射；对应页若因文件截断而不再有效，访问可能得到 `SIGBUS`。顺序一次性扫描的简单任务不必为了‘零拷贝’增加复杂度。

## 从代码看执行顺序

完整源码如下；同一份代码也在阅读版和 `.cpp` 中，改动时须三处同步。

```cpp
// 目的：安全演示只读文件映射；不访问越界，也不触发 SIGBUS。
// 步骤：建文件 → 写 ABCD → mmap 只读私有页 → 读第 3 字节 → 解映射。
// 观察：映射和文件描述符是不同资源，分别 munmap 与 close。
#include <cstdlib>      // mkstemp
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <sys/mman.h>   // mmap、munmap、PROT_READ、MAP_PRIVATE
#include <unistd.h>     // write、close、unlink

int main() {
    char path[] = "/tmp/bagu_map_XXXXXX";
    const int fd = ::mkstemp(path);
    if (fd == -1) throw std::runtime_error("mkstemp failed");
    const char data[] = "ABCD";
    if (::write(fd, data, 4) != 4) throw std::runtime_error("write failed");
    // nullptr 让内核选地址；4 是映射长度；0 是页对齐的文件偏移。
    void* mapped = ::mmap(nullptr, 4, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) throw std::runtime_error("mmap failed");
    const auto* bytes = static_cast<const char*>(mapped); // 只读指针防误写。
    std::cout << bytes[2] << '\n'; // 第 3 字节是 C；下标从 0 开始。
    if (::munmap(mapped, 4) == -1) throw std::runtime_error("munmap failed");
    if (::close(fd) == -1 || ::unlink(path) == -1)
        throw std::runtime_error("cleanup failed");
}
```

逐步追踪：

1. `mkstemp` 创建文件，`write` 填入确定内容。
2. `mmap(..., PROT_READ, MAP_PRIVATE, fd, 0)` 建立只读私有映射。
3. 读取 `bytes[2]` 时可能触发页面准备，具体是否缺页由当前状态决定。
4. `munmap` 释放映射，`close` 释放 fd；两个生命周期分开。

**输出与保证：**预期输出 `C`。访问成功不说明映射总比 read 快；示例不测时延。

## 把关系画出来

```mermaid
flowchart LR
  A["文件页"] --> B["read 路径"]
  B --> C["应用缓冲区"]
  A --> D["mmap 映射"]
  D --> E["进程虚拟地址"]
  E --> F["访问时可能缺页/I/O"]
```

图中箭头表示本节讨论的状态或依赖；它不是硬件时序图。

## 在项目里什么时候用

只读、随机访问的大型 hint/index 文件可评估 mmap；短小顺序读用 read 更直接。性能决定应在相同工作负载与缓存条件下测量。

## 易错边界与反例

- 映射长度不应随意超过有意义的文件数据；访问被截断后的页可能 SIGBUS。
- `MAP_PRIVATE` 的修改是私有写时复制，不构成文件更新。
- `MAP_SHARED` 修改不是‘自动立即落盘’；显式持久化需要核对 `msync`/`fsync` 与文件系统语义。

## 换一个条件自己试

把示例改成跨进程共享只读索引时，哪些量可记录在文件中，为什么不能直接把进程内指针写入文件？

## 30 秒口述

> mmap 是虚拟地址映射，read 是显式读入缓冲；按访问模式和缺页成本选择，不能保证 mmap 更快，截断映射要防 SIGBUS。

## 来源与适用范围

**题型来源：**[字节跳动 C++ 数据库开发实习 ByteHouse 面经](https://www.nowcoder.com/discuss/572859889337335808)明确记录了‘使用 mmap 读取 hint 文件构建索引’；‘总是更快’和截断追问为本库增加。

**技术核对（2026-10-26 访问）：**
- [Linux mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html)
- [Linux msync(2)](https://man7.org/linux/man-pages/man2/msync.2.html)

本节代码面向 Linux/POSIX，语言基线为 C++17；涉及文件系统与设备的结论需以运行环境为准。
