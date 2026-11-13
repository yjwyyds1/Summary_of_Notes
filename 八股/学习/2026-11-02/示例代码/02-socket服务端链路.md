# 2026-11-02 · 服务器怎样接入一条 TCP 连接：注释源码

先修：[[学习/2026-11-02/02-socket服务端链路|对应专题]]。此程序为独立 C++17 教学示例，成功时 `assert` 无输出。

```cpp
// 目的：在本机回环地址演示监听 fd、客户端 fd 与 accept 返回的连接 fd。
// 前置：Linux/POSIX 允许本机 TCP loopback；此处使用阻塞 API，只服务一个客户端。
// 步骤：绑定随机端口，监听，客户端连接，接受并收取一个字节，最后分别关闭三个 fd。
#include <arpa/inet.h>  // htons：把端口转换为网络字节序。
#include <cassert>      // assert：检查这个教学示例的系统调用结果。
#include <sys/socket.h> // socket、bind、listen、connect、accept、send、recv。
#include <unistd.h>     // close：释放文件描述符。

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0); // 用于监听，尚未代表某个客户端。
    assert(listener >= 0);
    sockaddr_in addr{};                 // IPv4 地址结构清零。
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 只允许本机连接。
    addr.sin_port = htons(0);          // 0 表示让内核挑一个可用端口。
    assert(bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    assert(listen(listener, 4) == 0);   // 开始监听，4 是队列参数而非用户上限。
    socklen_t length = sizeof(addr);
    assert(getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &length) == 0);
    int client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client >= 0);
    assert(connect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    int peer = accept(listener, nullptr, nullptr); // 新 fd 对应刚建立的这条连接。
    assert(peer >= 0 && peer != listener);
    const char sent = 'A';
    assert(send(client, &sent, 1, 0) == 1);
    char received = 0;
    assert(recv(peer, &received, 1, 0) == 1 && received == 'A');
    close(peer);   // 只关闭这一位客户端的服务端连接端点。
    close(client); // 关闭客户端端点。
    close(listener); // 最后关闭监听端点。
}
```

观察：先自己预测每个断言检查什么，再按 [[学习/2026-11-02/示例验证|验证记录]] 编译运行。
