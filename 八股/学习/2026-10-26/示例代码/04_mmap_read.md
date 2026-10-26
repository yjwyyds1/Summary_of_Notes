---
date: 2026-10-26
type: 示例代码阅读版
id: IO-MMAP-001
---

# 04_mmap_read · mmap 与 read：按访问方式选接口

用途：订单簿快照、索引和历史行情回放访问方式不同：顺序扫、随机跳、是否共享、是否会被改写。选接口时应看这些条件，而不是背‘零拷贝一定快’。

先读：[[学习/2026-10-26/04-mmap与read|专题详解]]。

## 与 cpp 逐字一致的完整源码

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

## 读完后核对

- 预期输出 `C`。访问成功不说明映射总比 read 快；示例不测时延。
- `mkstemp` 创建文件，`write` 填入确定内容。
- `mmap(..., PROT_READ, MAP_PRIVATE, fd, 0)` 建立只读私有映射。
- 读取 `bytes[2]` 时可能触发页面准备，具体是否缺页由当前状态决定。
- `munmap` 释放映射，`close` 释放 fd；两个生命周期分开。

- 自测变式：把示例改成跨进程共享只读索引时，哪些量可记录在文件中，为什么不能直接把进程内指针写入文件？
