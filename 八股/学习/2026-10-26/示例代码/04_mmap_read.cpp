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
