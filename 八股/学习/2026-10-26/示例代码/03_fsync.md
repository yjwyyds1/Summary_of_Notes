---
date: 2026-10-26
type: 示例代码阅读版
id: IO-DURABILITY-001
---

# 03_fsync · write 返回与 fsync：数据何时算持久

用途：订单或成交日志若先告知上游‘已成功’，却没有定义何时确认为持久，故障后的恢复范围会变得含糊。先约定成功语义，再决定同步时机。

先读：[[学习/2026-10-26/03-页缓存与fsync|专题详解]]。

## 与 cpp 逐字一致的完整源码

```cpp
// 目的：演示普通写入后显式同步文件的调用顺序（Linux/POSIX）。
// 步骤：建临时文件 → 写数据 → fsync → 删除临时文件；不做掉电实验。
// 观察：成功打印表示本机调用返回成功，不证明任意硬件故障下零丢失。
#include <cerrno>       // errno、EINTR
#include <cstdlib>      // mkstemp
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <unistd.h>     // write、fsync、close、unlink

int main() {
    char path[] = "/tmp/bagu_sync_XXXXXX";
    const int fd = ::mkstemp(path);
    if (fd == -1) throw std::runtime_error("mkstemp failed");
    const char record[] = "order=42\n";
    std::size_t done = 0;
    constexpr std::size_t size = sizeof(record) - 1; // 不写末尾 NUL。
    while (done < size) {
        const ssize_t n = ::write(fd, record + done, size - done);
        if (n > 0) { done += static_cast<std::size_t>(n); continue; }
        if (n == -1 && errno == EINTR) continue;
        throw std::runtime_error("write failed");
    }
    if (::fsync(fd) == -1) throw std::runtime_error("fsync failed");
    std::cout << "file fsync returned successfully\n";
    if (::close(fd) == -1) throw std::runtime_error("close failed");
    if (::unlink(path) == -1) throw std::runtime_error("unlink failed");
}
```

## 读完后核对

- 预期打印 `file fsync returned successfully`。示例随后主动删除文件，目的只是看调用边界。
- `write_all` 负责把业务字节全部提交给内核；这一步只解决短写。
- `fsync(fd)` 同步该打开文件对应的数据和元数据，并检查返回值。
- 若文件是刚创建且要保证路径名可恢复，还要打开父目录并对目录 fd 做 fsync。
- 示例删除临时路径，仅演示调用顺序，不宣称验证了断电恢复。

- 自测变式：假设 write 返回成功后断电、fsync 返回后断电、rename 后未同步目录时断电，分别写出能确定和不能确定的部分。
