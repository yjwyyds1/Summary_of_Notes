// C++17 / Linux 示例：用管道模拟可读 fd，观察 epoll ET 的读到 EAGAIN。
// Windows 分支仅说明平台限制，不声称已验证 Linux 分支。
#include <iostream> // cout/cerr：输出结果或错误。

#ifdef __linux__
#include <cerrno>       // errno、EAGAIN：识别“暂时读完”。
#include <fcntl.h>      // fcntl、O_NONBLOCK：设置非阻塞。
#include <stdexcept>    // runtime_error：系统调用失败时停止示例。
#include <sys/epoll.h>  // epoll_create1/ctl/wait 与事件类型。
#include <unistd.h>     // pipe、read、write、close。

class Fd {
    int value_; // Linux 文件描述符是非负整数；-1 表示无效。
public:
    explicit Fd(int value) : value_(value) {} // 获取这个描述符的关闭责任。
    ~Fd() { if (value_ >= 0) close(value_); } // 离开作用域自动关闭。
    Fd(const Fd&) = delete;             // 禁止复制，防止两个对象重复 close。
    Fd& operator=(const Fd&) = delete;
    int get() const { return value_; } // 借用数值给系统调用，不转移所有权。
};

int main() {
    int ends[2]; // ends[0] 读端，ends[1] 写端。
    if (pipe(ends) == -1) throw std::runtime_error("pipe failed");
    Fd reader(ends[0]);
    Fd writer(ends[1]);
    const int flags = fcntl(reader.get(), F_GETFL, 0); // 读取当前状态标志。
    if (flags == -1 || fcntl(reader.get(), F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl failed");
    Fd epoll_fd(epoll_create1(EPOLL_CLOEXEC));
    if (epoll_fd.get() == -1) throw std::runtime_error("epoll_create1 failed");
    epoll_event wanted{}; // 零初始化其它字段。
    wanted.events = EPOLLIN | EPOLLET; // 可读 + 边缘触发；去掉 EPOLLET 即 LT。
    wanted.data.fd = reader.get();     // 返回事件时定位是哪个 fd。
    if (epoll_ctl(epoll_fd.get(), EPOLL_CTL_ADD, reader.get(), &wanted) == -1)
        throw std::runtime_error("epoll_ctl failed");
    if (write(writer.get(), "AB", 2) != 2) throw std::runtime_error("write failed");
    epoll_event happened{};
    const int count = epoll_wait(epoll_fd.get(), &happened, 1, 1000);
    if (count != 1 || !(happened.events & EPOLLIN))
        throw std::runtime_error("expected readable event");
    char buffer[8];
    for (;;) { // ET 下读到暂时无数据，再等待下一次边缘。
        const auto n = read(reader.get(), buffer, sizeof buffer);
        if (n > 0) { std::cout.write(buffer, n); continue; }
        if (n == -1 && errno == EINTR) continue; // 信号打断，本次操作重试。
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (n == 0) break; // 管道写端关闭，读到 EOF。
        throw std::runtime_error("read failed");
    }
    std::cout << " drained to EAGAIN\n";
}
#else
int main() {
    std::cout << "This example requires Linux epoll; read the annotated Linux branch.\n";
}
#endif
