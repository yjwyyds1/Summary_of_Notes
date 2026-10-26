---
date: 2026-10-26
type: 示例代码阅读版
id: IO-FD-001
---

# 01_fd_offset · 文件描述符、打开文件描述与共享偏移

用途：日志轮转、父子进程、库函数接收 fd 时，若误以为每个整数都独立维护位置，读取可能跳过数据。要判断共享什么，先找 fd 是否指向同一个打开文件描述。

先读：[[学习/2026-10-26/01-文件描述符与共享偏移|专题详解]]。

## 与 cpp 逐字一致的完整源码

```cpp
// 目的：观察两次 open 与 dup 在文件偏移上的差异（Linux/POSIX，C++17）。
// 步骤：创建临时文件 → 写入 ABCDEFGH → 复制/重新打开 → 顺序读取。
// 观察：first 读 AB 后，duplicate 读 CD；independent 仍从头读 AB。
#include <fcntl.h>      // open、O_RDONLY
#include <unistd.h>     // read、write、lseek、dup、close、unlink
#include <cstdlib>      // mkstemp
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <string>       // string

int main() {
    char path[] = "/tmp/bagu_fd_XXXXXX"; // mkstemp 会把 X 替换为唯一后缀。
    const int first = ::mkstemp(path);    // 返回可读写 fd，失败为 -1。
    if (first == -1) throw std::runtime_error("mkstemp failed");
    const char data[] = "ABCDEFGH";
    if (::write(first, data, 8) != 8 || ::lseek(first, 0, SEEK_SET) == -1)
        throw std::runtime_error("prepare failed");
    const int duplicate = ::dup(first);   // 新 fd 指向 first 的同一打开文件描述。
    const int independent = ::open(path, O_RDONLY); // 另一次 open：独立偏移。
    if (duplicate == -1 || independent == -1)
        throw std::runtime_error("open or dup failed");
    ::unlink(path); // 只移除目录名；已打开的 fd 仍可访问此文件。
    auto read_two = [](int fd) {
        char bytes[3]{}; // 多留一字节给 C 字符串终止符 0。
        if (::read(fd, bytes, 2) != 2) throw std::runtime_error("read failed");
        return std::string(bytes, 2); // 指定长度，避免依赖隐式终止符。
    };
    std::cout << read_two(first) << ' ' << read_two(duplicate) << ' '
              << read_two(independent) << '\n';
    ::close(independent);
    ::close(duplicate);
    ::close(first);
}
```

## 读完后核对

- 示例的预期输出是 `AB CD AB`；这里用普通文件，长度固定且逐次读取。真实接口仍要检查短读写、错误和关闭失败。
- `open(path, O_RDWR)`：创建第一个打开文件描述，初始偏移为 0。
- `dup(first)`：只增加一张 fd 表入口，和 first 共享偏移。
- `open(path, O_RDONLY)`：第二次打开同一个文件，新建独立打开文件描述。
- 从 first 读 2 字节后，duplicate 从第 3 字节继续读；independent 从第 1 字节读。

- 自测变式：把 `dup` 改为第二次 `open`，预测第二次读取的内容；再解释 `fork` 继承时哪些 fd 共享偏移。
