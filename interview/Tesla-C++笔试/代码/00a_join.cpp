#include <iostream>
#include <thread>

void work() {
    std::cout << "1. worker is running\n";
}

int main() {
    std::thread worker(work); // 启动线程，在新线程里调用 work。
    worker.join();             // main 等 work 执行完，才继续往下。
    std::cout << "2. main can finish\n";
}
