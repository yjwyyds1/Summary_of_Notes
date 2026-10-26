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
