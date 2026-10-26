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
