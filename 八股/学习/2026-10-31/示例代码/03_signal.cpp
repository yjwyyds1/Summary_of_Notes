// 目的：观察“信号处理器只记下请求，普通执行流程再处理请求”。
// 平台：Linux + C++17；使用 POSIX sigaction，不能直接当成纯标准 C++ 跨平台代码。
// 先修：函数、全局变量、if、for，以及 &obj 表示取得对象地址。
// 步骤：安装处理器 -> 确保 SIGUSR1 未被阻塞 -> 自己发信号 -> 主流程退出循环。
// 观察：全部输出都来自 main；处理器没有输出、分配内存或加锁。
// 程序不向其他进程发信号，不创建线程或子进程，不需要外部输入。

#include <csignal>   // std::sig_atomic_t、std::raise：信号标记类型与给自己发信号。
#include <cstdio>    // std::perror：仅在普通执行流程中打印系统调用错误。
#include <iostream>  // std::cout/std::cerr：普通流程的教学输出。
#include <signal.h>  // POSIX 的 sigaction、sigset_t、sigprocmask 等接口。

// 0 表示尚未请求停止，1 表示请求过停止。
// volatile sig_atomic_t 适合本例“同一线程的处理器与正常流程”之间的简单标记。
// 它不是 std::atomic，也不能据此推导 volatile 能解决多个线程的数据竞争。
volatile std::sig_atomic_t stop_requested = 0;

// extern "C" 指定 C 语言链接，匹配信号处理回调的接口形式。
// 参数 int 是收到的信号编号；本例只注册一种信号，不使用它，所以省略参数名。
// void 表示没有返回值；这里不调用库函数，只把停止标记置为 1。
extern "C" void on_signal(int) {
    stop_requested = 1;
}

int main() {
    // {} 先把结构体各成员初始化；struct 是为了明确这是类型而不是同名函数。
    struct sigaction action{};
    // 这里只保存函数入口地址，没有立即调用 on_signal。
    action.sa_handler = on_signal;
    action.sa_flags = 0;  // 使用最基本的处理方式，不启用额外行为标志。

    // sa_mask 是处理器执行期间额外阻塞的信号集合；用 API 初始化为空集合。
    if (::sigemptyset(&action.sa_mask) == -1) {
        std::perror("sigemptyset(action)");
        return 1;
    }
    // &action 让内核读取这份配置；nullptr 表示本例不保存原来的处理方式。
    if (::sigaction(SIGUSR1, &action, nullptr) == -1) {
        std::perror("sigaction");
        return 1;
    }

    // 信号屏蔽状态可从启动者继承。显式解除本例信号的阻塞，避免它一直待处理。
    // 必须先安装处理器，再解除阻塞；SIGUSR1 的默认动作是终止进程。
    sigset_t unblocked{};
    if (::sigemptyset(&unblocked) == -1) {
        std::perror("sigemptyset(unblocked)");
        return 1;
    }
    if (::sigaddset(&unblocked, SIGUSR1) == -1) {
        std::perror("sigaddset");
        return 1;
    }
    // 本例只有一个线程，所以使用 sigprocmask；多线程程序应另行设计信号接收。
    if (::sigprocmask(SIG_UNBLOCK, &unblocked, nullptr) == -1) {
        std::perror("sigprocmask");
        return 1;
    }

    // 最多执行五个模拟工作单元；第二个单元后给当前线程发送 SIGUSR1。
    for (int unit = 1; unit <= 5; ++unit) {
        std::cout << "完成工作单元 " << unit << '\n';
        if (unit == 2) {
            // 信号未被阻塞且已安装处理器；raise 在处理器返回后才返回。
            // raise 返回 0 表示成功，非 0 表示失败；不假定失败一定设置 errno。
            if (std::raise(SIGUSR1) != 0) {
                std::cerr << "raise 失败\n";
                return 1;
            }
        }
        if (stop_requested != 0) {
            // 此时已经回到 main，可以正常使用输出、RAII 与业务清理代码。
            std::cout << "主流程收到停止请求，不再开始下一个工作单元\n";
            break;  // 只退出 for，继续执行 main 后面的清理位置。
        }
    }

    std::cout << "主流程执行收尾并正常返回\n";
    return 0;  // 正常离开 main 的作用域；本例没有需要回收的线程或子进程。
}
