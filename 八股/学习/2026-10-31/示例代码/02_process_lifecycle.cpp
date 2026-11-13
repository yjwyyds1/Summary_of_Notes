// Linux + C++17 教学示例：fork 创建子进程，execve 替换程序，waitpid 回收结果。
// 前置：fork 返回值见 2026-10-24；本例新增程序替换、退出状态与回收责任。
// 运行条件：Linux 已挂载 /proc，允许执行 /proc/self/exe；以单线程方式运行。
// 步骤：成功 exec 自身并返回 42；再演示 exec 失败，以约定的 127 退出。
// 每轮只创建一个子进程，并在开始下一轮前回收；没有 sleep、后台任务或文件写入。
#include <cerrno>   // errno、EINTR、ENOENT、ECHILD：分辨中断和真正错误。
#include <cstddef>  // std::size_t：记录错误提示已经输出的字节数。
#include <cstdio>   // printf、fflush、perror：父进程与 exec 后的新程序输出。
#include <cstring>  // strcmp：识别 --child，防止 exec 后再次创建子进程。
#include <sys/types.h> // pid_t：进程编号类型。
#include <sys/wait.h>  // waitpid 和 WIFEXITED/WEXITSTATUS/WIFSIGNALED 等状态宏。
#include <unistd.h>    // fork、execve、getpid、write、_exit。

bool run_child(const char* executable, int expected_exit) {
    // execve 的 argv 是以空指针结束的“字符串指针数组”。
    // 先准备可修改字符数组，匹配 API 的 char* 类型，不强转字符串字面量。
    char name[] = "process_lifecycle";
    char mode[] = "--child";
    char* arguments[] = {name, mode, nullptr};
    char* environment[] = {nullptr}; // 教学程序不依赖环境变量，传空环境。

    // 先冲刷已有 stdio 缓冲，避免 fork 复制未输出文本后父子重复冲刷。
    if (std::fflush(nullptr) != 0) {
        std::perror("fflush before fork");
        return false; // 此时尚未创建子进程，不存在回收义务。
    }
    const pid_t child = ::fork();
    if (child == -1) {
        std::perror("fork");
        return false;
    }
    if (child == 0) {
        // 此分支只由子进程执行。成功后从新程序入口开始，绝不会执行下一行。
        ::execve(executable, arguments, environment);

        // 走到这里说明 execve 失败，原程序映像还在，errno 说明失败原因。
        const int exec_error = errno;
        const char message[] = "child: execve failed; exiting without continuing parent logic\n";
        // fork 后仅做简单系统接口调用；错误提示失败也不能阻止子进程退出。
        std::size_t offset = 0;
        while (offset < sizeof(message) - 1) {
            const ssize_t n = ::write(STDERR_FILENO, message + offset,
                                      sizeof(message) - 1 - offset);
            if (n > 0) {
                offset += static_cast<std::size_t>(n);
            } else if (n == -1 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        // 127/126 是本例选择的报告约定，不是 execve 自动生成的退出码。
        // _exit 不冲刷复制来的 stdio 缓冲，也不回到父进程那段业务逻辑。
        ::_exit(exec_error == ENOENT ? 127 : 126);
    }

    // 父进程只等待刚创建的这一个子进程；&status 是把结果写入的位置。
    int status = 0; // 编码后的状态字，不能直接把它当成子进程返回值。
    pid_t waited;
    do {
        waited = ::waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR); // 信号打断等待后，回收义务仍在。
    if (waited == -1) {
        std::perror("waitpid");
        return false;
    }

    // waitpid 成功返回后，本子进程已被回收；先判断退出类别，再解码。
    if (!WIFEXITED(status)) {
        if (WIFSIGNALED(status)) {
            std::fprintf(stderr, "child terminated by signal %d\n", WTERMSIG(status));
        }
        return false;
    }
    const int exit_code = WEXITSTATUS(status);
    std::printf("parent reaped pid=%ld, exit=%d\n", static_cast<long>(child), exit_code);
    if (exit_code != expected_exit) {
        return false;
    }

    // 再等待同一已回收子进程应得到 ECHILD，证明“读取结果”和“回收”已完成。
    errno = 0;
    const pid_t again = ::waitpid(child, &status, WNOHANG);
    if (again != -1 || errno != ECHILD) {
        std::fprintf(stderr, "unexpected second waitpid result\n");
        return false;
    }
    std::puts("second waitpid: ECHILD (already reaped)");
    return true;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::strcmp(argv[1], "--child") == 0) {
        // 这是 exec 成功后的新程序，PID 沿用 fork 子进程的 PID。
        std::printf("exec child pid=%ld, returning 42\n", static_cast<long>(::getpid()));
        return 42; // 正常从 main 返回，父进程通过 WEXITSTATUS 取得 42。
    }
    if (!run_child("/proc/self/exe", 42)) {
        return 1;
    }
    // 空路径在 Linux 上不能指向可执行文件：明确演示 ENOENT 失败分支。
    if (!run_child("", 127)) {
        return 1;
    }
    return 0; // 两个子进程都已经回收，没有特意留下僵尸或孤儿。
}
