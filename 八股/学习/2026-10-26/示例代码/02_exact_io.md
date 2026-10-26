---
date: 2026-10-26
type: 示例代码阅读版
id: IO-PARTIAL-001
---

# 02_exact_io · read/write 为什么要处理短读写

用途：日志记录若只调用一次 write 就把返回当成功，可能只存下半条前缀。文件回放若只 read 一次就当作整条消息，也会把尚未读到的字节误判成坏数据。

先读：[[学习/2026-10-26/02-短读写与错误处理|专题详解]]。

## 与 cpp 逐字一致的完整源码

```cpp
// 目的：用统一的进度变量正确处理短读写（Linux/POSIX，C++17）。
// 步骤：创建临时文件 → 写完整 6 字节 → 定位开头 → 读完整 6 字节。
// 观察：普通文件本次可能每次都返回足量；循环是为其他合法返回情况准备。
#include <cerrno>       // errno、EINTR
#include <cstdlib>      // mkstemp
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <unistd.h>     // read、write、lseek、close、unlink

void write_all(int fd, const char* data, std::size_t size) {
    std::size_t done = 0; // 只记录已经成功写入的字节。
    while (done < size) {
        const ssize_t n = ::write(fd, data + done, size - done);
        if (n > 0) { done += static_cast<std::size_t>(n); continue; }
        if (n == -1 && errno == EINTR) continue; // 没有进度，重试剩余部分。
        throw std::runtime_error("write did not finish"); // 包含 0/其他错误。
    }
}

bool read_exact(int fd, char* data, std::size_t size) {
    std::size_t done = 0;
    while (done < size) {
        const ssize_t n = ::read(fd, data + done, size - done);
        if (n > 0) { done += static_cast<std::size_t>(n); continue; }
        if (n == 0) return false; // EOF：上层知道记录不完整。
        if (errno == EINTR) continue;
        throw std::runtime_error("read failed");
    }
    return true;
}

int main() {
    char path[] = "/tmp/bagu_exact_XXXXXX";
    const int fd = ::mkstemp(path);
    if (fd == -1) throw std::runtime_error("mkstemp failed");
    ::unlink(path);
    const char message[] = "ABCDEF"; // 不写末尾的字符串终止符。
    write_all(fd, message, 6);
    if (::lseek(fd, 0, SEEK_SET) == -1) throw std::runtime_error("seek failed");
    char result[6]{};
    if (!read_exact(fd, result, 6)) throw std::runtime_error("unexpected EOF");
    std::cout.write(result, 6); // 指定长度，不要求 result 是 C 字符串。
    std::cout << '\n';
    ::close(fd);
}
```

## 读完后核对

- 预期打印 `ABCDEF`。这次运行不强制制造短写；函数正确性依赖逐次检查返回值的控制流。
- `done` 初始为 0，始终表示已处理的字节数。
- `data + done` 从尚未处理的首字节开始；`size - done` 是剩余量。
- 正数返回后只增加 `done`；`EINTR` 重新尝试同一剩余区间。
- 读到 EOF 时若 `done < size`，返回不足，交给上层判定截断记录。

- 自测变式：让写入函数在第 37 字节后模拟一次短写，写出第二轮传入的指针和长度；不要假设运行普通文件必定触发短写。
