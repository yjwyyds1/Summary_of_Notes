---
date: 2026-10-26
type: 示例代码阅读版
id: IO-RENAME-001
---

# 05_atomic_replace · 临时文件、rename 与目录 fsync

用途：回放索引、配置和状态快照可能有读者同时打开。直接 `O_TRUNC` 改正式文件会出现可见的空文件或前半段，因而需要先写新版本，再切换路径名。

先读：[[学习/2026-10-26/05-临时文件与原子替换|专题详解]]。

## 与 cpp 逐字一致的完整源码

```cpp
// 目的：演示快照文件的“先写新版本 → 同步 → 切换名字”顺序。
// 前提：Linux/POSIX；演示目录在 /tmp；不声明任意存储设备上的绝对掉电保证。
// 观察：target 最终包含 new-snapshot；临时目录会被清理。
#include <cerrno>       // errno、EINTR
#include <cstdlib>      // mkdtemp
#include <fcntl.h>      // open、O_DIRECTORY、O_RDONLY、O_WRONLY
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <string>       // string
#include <unistd.h>     // write、fsync、close、read、rmdir
#include <cstdio>       // rename

int main() {
    char dir_template[] = "/tmp/bagu_replace_XXXXXX";
    char* dir = ::mkdtemp(dir_template); // 唯一演示目录，避免覆盖真实文件。
    if (!dir) throw std::runtime_error("mkdtemp failed");
    const std::string temp = std::string(dir) + "/snapshot.tmp";
    const std::string target = std::string(dir) + "/snapshot.dat";
    const int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd == -1) throw std::runtime_error("open temp failed");
    const std::string body = "new-snapshot";
    std::size_t done = 0;
    while (done < body.size()) {
        const ssize_t n = ::write(fd, body.data() + done, body.size() - done);
        if (n > 0) { done += static_cast<std::size_t>(n); continue; }
        if (n == -1 && errno == EINTR) continue;
        throw std::runtime_error("write failed");
    }
    if (::fsync(fd) == -1 || ::close(fd) == -1)
        throw std::runtime_error("file sync/close failed");
    if (::rename(temp.c_str(), target.c_str()) == -1)
        throw std::runtime_error("rename failed");
    const int dir_fd = ::open(dir, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1 || ::fsync(dir_fd) == -1 || ::close(dir_fd) == -1)
        throw std::runtime_error("directory sync failed");
    const int read_fd = ::open(target.c_str(), O_RDONLY);
    if (read_fd == -1) throw std::runtime_error("open target failed");
    char buffer[32]{};
    const ssize_t n = ::read(read_fd, buffer, sizeof(buffer));
    if (n < 0) throw std::runtime_error("read target failed");
    std::cout.write(buffer, n);
    std::cout << '\n';
    ::close(read_fd);
    ::unlink(target.c_str());
    ::rmdir(dir);
}
```

## 读完后核对

- 预期输出 `new-snapshot`。目标文件最初不存在，示例演示可见切换的 API 次序；实际替换旧文件时还要考虑并发写者。
- `mkdtemp` 建私有演示目录；实际程序应选择目标文件的同一文件系统。
- 写临时文件并 `fsync(temp_fd)`，先让新内容达到文件同步点。
- `rename(temp, target)` 切换路径可见对象。
- 打开目录后 `fsync(dir_fd)`，处理目录项持久化；最后读取 target 验证正常路径。

- 自测变式：若把 fsync(temp) 移到 rename 后、或者只 fsync 文件不 fsync 目录，指出哪一种保证失去依据。
