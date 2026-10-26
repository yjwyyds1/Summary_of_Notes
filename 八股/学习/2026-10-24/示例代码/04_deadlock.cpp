// C++17 安全示例：一次转账要同时占有两把锁，使用 scoped_lock 避免相反加锁顺序。
// 不执行可能死锁的反例；示例能正常结束不等于所有转账系统都没有死锁。
#include <cassert>  // assert：检查钱的总量守恒。
#include <iostream> // cout：打印最后余额。
#include <mutex>    // mutex、scoped_lock：保护共享余额并一起获取两把锁。
#include <thread>   // thread：模拟两个同时转账的执行流。

struct Account {
    int balance;      // 钱数用整数，避免本例讨论浮点舍入。
    std::mutex mutex{}; // 每个账户有自己的互斥锁；默认成员初始化也避免聚合初始化警告。
};

void transfer(Account& from, Account& to, int amount) {
    // 两个引用表示要操作的实际账户；本题保证 from 与 to 不同。
    std::scoped_lock both(from.mutex, to.mutex); // C++17 一起获取，离开函数自动解锁。
    from.balance -= amount; // 持锁时更新两个余额，不让别的线程看到中间状态。
    to.balance += amount;
}

int main() {
    Account a{100};
    Account b{100};
    std::thread first([&] { for (int i = 0; i < 100; ++i) transfer(a, b, 1); });
    std::thread second([&] { for (int i = 0; i < 100; ++i) transfer(b, a, 1); });
    first.join();  // 等两个线程都结束，才读最终余额。
    second.join();
    assert(a.balance + b.balance == 200);
    std::cout << "final balances: " << a.balance << ' ' << b.balance << '\n';
}
