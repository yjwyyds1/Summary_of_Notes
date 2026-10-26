# 02_pipe_ipc — 完整注释源码

← [[学习/2026-10-24/示例代码/README|代码索引]] · [[学习/2026-10-24/02-进程间通信|主题详解]]

以下代码与同名 .cpp 逐字一致；完整编译与输出见 [[学习/2026-10-24/示例验证]]。

```cpp
// C++17 / Linux 示例：父子进程用匿名管道传递三个字节。
// Windows 上只显示平台提示；管道是字节流，不保证一次 write 对应一次 read。
#include <iostream> // cout/cerr：结果与错误信息。
#ifdef __linux__
#include <sys/wait.h> // waitpid：回收子进程，避免僵尸状态。
#include <unistd.h>  // pipe、fork、read、write、close、_exit。
#endif

int main() {
#ifdef __linux__
    int ends[2]; // ends[0] 是读端，ends[1] 是写端。
    if (pipe(ends) == -1) { std::cerr << "pipe failed\n"; return 1; }
    const pid_t child = fork();
    if (child == -1) { close(ends[0]); close(ends[1]); return 1; }
    if (child == 0) {
        close(ends[0]); // 子进程不读；关闭不用的一端。
        const char data[3] = {'A', 'B', 'C'};
        const ssize_t sent = write(ends[1], data, sizeof data);
        close(ends[1]); // 关闭最后写端，父进程读完后能看到 EOF。
        _exit(sent == 3 ? 0 : 1);
    }
    close(ends[1]); // 父进程不写，否则自己的写端会妨碍判断 EOF。
    char buffer[3]{};  // 足够存三个字节，无需 C 字符串终止符。
    std::size_t used = 0;
    while (used < sizeof buffer) {
        const ssize_t n = read(ends[0], buffer + used, sizeof buffer - used);
        if (n < 0) { close(ends[0]); waitpid(child, nullptr, 0); return 1; }
        if (n == 0) break; // 所有写端已关闭，且管道中没有剩余数据。
        used += static_cast<std::size_t>(n); // read 可能只交付部分字节。
    }
    close(ends[0]);
    int status = 0;
    if (waitpid(child, &status, 0) != child) return 1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || used != 3) return 1;
    std::cout << "parent received: ";
    std::cout.write(buffer, static_cast<std::streamsize>(used));
    std::cout << '\n';
#else
    std::cout << "pipe/fork example requires Linux\n";
#endif
}
```
