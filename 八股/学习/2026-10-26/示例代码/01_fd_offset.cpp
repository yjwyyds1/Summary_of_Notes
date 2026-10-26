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
