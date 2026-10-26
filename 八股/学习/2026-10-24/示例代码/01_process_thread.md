# 01_process_thread — 完整注释源码

← [[学习/2026-10-24/示例代码/README|代码索引]] · [[学习/2026-10-24/01-进程与线程|主题详解]]

以下代码与同名 .cpp 逐字一致；完整编译与输出见 [[学习/2026-10-24/示例验证]]。

```cpp
// C++17 教学示例：线程能修改同一进程的共享对象；Linux fork 后子进程修改的是自己的地址空间。
// Linux fork 分支不在 Windows 执行；代码不打印真实地址，也不把相同虚拟地址误说成同一物理内存。
#include <cassert>  // assert：核对预期结果。
#include <iostream> // cout：展示最终数值。
#include <thread>   // std::thread：创建并等待一个线程。
#ifdef __linux__
#include <sys/wait.h> // waitpid、WIFEXITED、WEXITSTATUS：回收子进程并读退出码。
#include <unistd.h>  // fork、_exit：创建进程与结束子进程。
#endif

int main() {
    int shared_in_process = 1; // main 与下面的线程属于同一个进程。
    std::thread worker([&shared_in_process] {
        // 引用捕获让线程访问 main 中同一个 int；join 前该对象一直有效。
        shared_in_process = 2;
    });
    worker.join(); // 等线程结束，建立同步；之后 main 才读这个值。
    assert(shared_in_process == 2);
    std::cout << "thread changed parent value: " << shared_in_process << '\n';

#ifdef __linux__
    int parent_value = 1; // fork 后父、子各有自己的虚拟地址空间。
    const pid_t child = fork();
    if (child == -1) { std::cerr << "fork failed\n"; return 1; }
    if (child == 0) {
        parent_value = 9; // 子进程改自己的副本；父进程中的变量仍是 1。
        _exit(parent_value); // 用退出码把 9 传回；避免子进程继续运行 main。
    }
    int status = 0;
    if (waitpid(child, &status, 0) != child) { std::cerr << "waitpid failed\n"; return 1; }
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 9);
    assert(parent_value == 1);
    std::cout << "parent after child exit: " << parent_value << '\n';
#else
    std::cout << "fork comparison requires Linux\n";
#endif
}
```
