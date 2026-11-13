// C++17 / Linux：把“启动一个程序并判断它怎样结束”封装成小函数。
// 前置：fork 创建进程，exec 替换程序，waitpid 取得并回收退出状态。
// 本程序只启动自己或尝试一个固定的不存在路径，不执行用户输入命令。
// 观察：退出 0、退出 7、被 SIGTERM 终止、exec 失败约定 127 是不同结果。
#include <cassert>   // assert：让已知样例的结果错误时立即暴露。
#include <cerrno>    // errno、EINTR：waitpid 被信号打断时重新等待。
#include <csignal>   // raise、SIGTERM：只向当前子进程自己发送信号。
#include <cstring>   // strcmp：比较命令行参数的字符串内容。
#include <iostream>  // cout：父进程在回收后打印结果。
#include <stdexcept> // runtime_error：把创建/等待失败交给调用方处理。
#include <sys/wait.h>// waitpid 和 WIF... / W...STATUS 等解码宏。
#include <unistd.h>  // fork、execl、_exit：POSIX 进程接口。

struct Result {
    bool by_signal; // true 时 value 是信号编号；false 时是正常退出码。
    int value;      // 不直接保存 waitpid 的原始 status，它包含编码信息。
};

Result run_child(const char* mode) {
    // const char*：借用调用者的字符串，函数不修改其中字符。
    const pid_t pid = fork();
    if (pid == -1) throw std::runtime_error("fork failed");
    if (pid == 0) {
        // child 分支：exec 成功后，此进程会从新程序 main 开始。
        // /proc/self/exe 是 Linux 暴露的当前可执行文件入口。
        const char* path = std::strcmp(mode, "--missing") == 0
                         ? "/proc/self/does-not-exist" : "/proc/self/exe";
        execl(path, "child-runner", mode, static_cast<char*>(nullptr));
        // execl 可变参数以一个空指针结束，不是字符 '\0'。
        // 到这一行意味着 exec 失败。127 是本例约定，不是 waitpid 自动生成。
        // _exit 不重复刷新 fork 继承的用户态流缓冲。
        _exit(127);
    }
    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0); // 只等刚创建的这个子进程。
    } while (waited == -1 && errno == EINTR);
    if (waited != pid) throw std::runtime_error("waitpid failed");
    if (WIFEXITED(status)) return {false, WEXITSTATUS(status)};
    if (WIFSIGNALED(status)) return {true, WTERMSIG(status)};
    throw std::runtime_error("unexpected child state");
}

int main(int argc, char* argv[]) {
    // exec 进入的子程序带模式参数；父程序无参数，走后面的测试代码。
    if (argc == 2) {
        if (std::strcmp(argv[1], "--ok") == 0) return 0;
        if (std::strcmp(argv[1], "--fail") == 0) return 7;
        if (std::strcmp(argv[1], "--signal") == 0) {
            std::raise(SIGTERM); // 默认动作终止当前子进程，不影响父进程。
            return 99;          // 正常情况下不会到这里，作为意外路径标记。
        }
        return 64;
    }
    try {
        const Result ok = run_child("--ok");
        const Result fail = run_child("--fail");
        const Result killed = run_child("--signal");
        const Result missing = run_child("--missing");
        assert(!ok.by_signal && ok.value == 0);
        assert(!fail.by_signal && fail.value == 7);
        assert(killed.by_signal && killed.value == SIGTERM);
        assert(!missing.by_signal && missing.value == 127);
        std::cout << "normal=" << ok.value << " failure=" << fail.value
                  << " signal=SIGTERM exec-failure=" << missing.value << '\n';
    } catch (const std::exception& error) {
        // const &：只读借用异常对象；错误信息写到标准错误。
        std::cerr << error.what() << '\n';
        return 1;
    }
}
