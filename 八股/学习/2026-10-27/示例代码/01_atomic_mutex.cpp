// 目的：区分单个计数的原子更新与多字段不变量的加锁更新。
// 前置：std::thread 运行任务；join 等待任务结束；不运行数据竞争反例。
// 观察：计数为 2000，cash + reserved 始终由锁保护。
#include <atomic>     // std::atomic：单个共享计数器的读改写。
#include <iostream>   // std::cout：打印结束后的结果。
#include <mutex>      // std::mutex、std::lock_guard：保护多字段约束。
#include <thread>     // std::thread：启动两个工作线程。

struct Account {
    int cash{10};       // 普通字段：只能在同一锁的保护下并发访问。
    int reserved{0};    // 两字段总和应保持 10。
    std::mutex guard;   // 所有参与者必须使用这个对象的同一把锁。

    void move_one() {
        std::lock_guard<std::mutex> lock(guard); // 退出函数时自动解锁。
        if (cash > 0) { --cash; ++reserved; }    // 判断与两次修改属于一个临界区。
    }
};

int main() {
    std::atomic<int> count{0}; // atomic 默认操作采用 seq_cst；下一节讨论内存序。
    Account account;
    auto work = [&] { // 按引用捕获；join 保证对象在任务完成前一直存活。
        for (int i = 0; i < 1000; ++i) count.fetch_add(1);
        for (int i = 0; i < 5; ++i) account.move_one();
    };
    std::thread a(work), b(work);
    a.join(); b.join(); // 两线程完成后，主线程才读结果。
    std::cout << "count=" << count.load()
              << " total=" << account.cash + account.reserved << '\n';
}
