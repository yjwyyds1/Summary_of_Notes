---
date: 2026-10-26
type: 示例代码阅读版
id: IO-WAL-001
---

# 06_wal_order · 预写日志 WAL、提交点与恢复

用途：订单处理流程若给上游回复持久成功，却仅修改内存且没有可恢复记录，进程崩溃会丢掉这笔确认过的变化；若主文件先持久化而日志尚未完整，也会破坏恢复判断。设计时把‘已收到’、‘已写内核’、‘已持久’、‘已应用’分开。

先读：[[学习/2026-10-26/06-WAL与重启恢复|专题详解]]。

## 与 cpp 逐字一致的完整源码

```cpp
// 目的：演示一个教学版“先写日志并同步，再应用内存状态”的顺序。
// 限制：无校验、序号、版本与并发控制；不能当成真实订单 WAL。
// 观察：打印应用后的状态；本次正常运行不等于验证了崩溃恢复。
#include <cerrno>       // errno、EINTR
#include <cstdint>      // uint32_t
#include <cstdlib>      // mkstemp
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <string>       // string
#include <unistd.h>     // write、fsync、close、unlink

void append_all(int fd, const char* bytes, std::size_t size) {
    std::size_t done = 0;
    while (done < size) {
        const ssize_t n = ::write(fd, bytes + done, size - done);
        if (n > 0) { done += static_cast<std::size_t>(n); continue; }
        if (n == -1 && errno == EINTR) continue;
        throw std::runtime_error("append failed");
    }
}

int main() {
    char path[] = "/tmp/bagu_wal_XXXXXX";
    const int fd = ::mkstemp(path);
    if (fd == -1) throw std::runtime_error("mkstemp failed");
    const std::string command = "order=42";
    const std::uint32_t length = static_cast<std::uint32_t>(command.size());
    // 明确写出小端长度；跨机器格式不能直接写本机整数内存布局。
    const char header[4] = {static_cast<char>(length & 0xffu),
                            static_cast<char>((length >> 8) & 0xffu),
                            static_cast<char>((length >> 16) & 0xffu),
                            static_cast<char>((length >> 24) & 0xffu)};
    append_all(fd, header, 4);
    append_all(fd, command.data(), command.size());
    if (::fsync(fd) == -1) throw std::runtime_error("fsync failed");
    // 教学模型：同步成功后才改变内存中的可查询状态。
    std::string in_memory_order = command;
    std::cout << "applied " << in_memory_order << '\n';
    if (::close(fd) == -1 || ::unlink(path) == -1)
        throw std::runtime_error("cleanup failed");
}
```

## 读完后核对

- 预期输出 `applied order=42`。示例不真正模拟重启；恢复扫描在下一练习中实现。
- 把操作编码成记录，示例写 4 字节长度 + `order=42`。
- 写完记录调用 `fsync(fd)`，教学示例把它作为本机同步确认点。
- 同步成功后更新内存状态并打印；重启时读出头和正文才可重放。
- 生产格式需要校验、版本、序号及截断尾部处理；07 练习专门处理不完整尾部。

- 自测变式：在‘写日志前、写了一半、写全但未 fsync、fsync 成功后’四个点崩溃，分别说明允许确认什么。
