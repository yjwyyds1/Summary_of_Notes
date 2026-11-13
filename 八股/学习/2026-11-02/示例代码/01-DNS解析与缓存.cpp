// 目的：通过系统接口取得域名对应的候选 socket 地址，不自行实现 DNS。
// 前置：POSIX/Linux，localhost 在本机解析配置中可用；不依赖公网。
// 步骤：设置地址提示，调用 getaddrinfo，遍历结果，最后释放系统分配的链表。
#include <cassert>      // assert：检查至少取得一个候选地址。
#include <netdb.h>      // getaddrinfo/freeaddrinfo/gai_strerror 的声明。
#include <sys/socket.h> // AF_UNSPEC 与 SOCK_STREAM 的定义。

int main() {
    addrinfo hints{};              // 零初始化：没有额外限制的字段保持 0。
    hints.ai_family = AF_UNSPEC;   // 同时接受 IPv4 和 IPv6 候选项。
    hints.ai_socktype = SOCK_STREAM; // 只要适合流式 socket 的地址。
    addrinfo* result = nullptr;   // 系统会把结果链表的首地址写到这里。
    const int rc = getaddrinfo("localhost", "80", &hints, &result);
    if (rc != 0) return 1;        // 解析失败时不能继续解引用 result。
    int count = 0;
    for (addrinfo* p = result; p != nullptr; p = p->ai_next) {
        assert(p->ai_addr != nullptr); // 每项包含可供 connect 使用的地址结构。
        ++count;                      // 地址可能不止一个，顺序也不作保证。
    }
    freeaddrinfo(result);         // 释放整个链表，不能用普通 delete。
    assert(count > 0);
}
