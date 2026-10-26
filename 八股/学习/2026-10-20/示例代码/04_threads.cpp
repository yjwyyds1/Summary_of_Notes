// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 这份例子把三件事分开：
// 1. 每个线程持有自己的 shared_ptr 管理者副本，共同保持一个 Counter 活着。
// 2. Counter::value 是普通 int，shared_ptr 不会替它加锁；读改写必须靠 mutex 协调。
// 3. main 在两个工作线程 join 后再读取 value，此时没有工作线程再修改它。
// 本例明确使用互斥锁，不是无锁算法；引用计数安全也不等于被指对象自动线程安全。
// 正常输出：value = 20000。只在创建线程、加锁等操作成功的演示路径下观察此输出。

#include <iostream>  // std::cout 输出两个线程完成后的最终计数。
#include <memory>    // shared_ptr/make_shared：多个拥有者共同管理一个 Counter 的生命周期。
#include <mutex>     // mutex 提供互斥；lock_guard 把解锁绑定到作用域结束。
#include <thread>    // thread 启动线程，join 等待线程完成，joinable 检查是否需要收尾。

struct Counter {
    // 这把锁与 value 放在一起，提醒调用者：访问这份共享计数时使用同一把锁。
    std::mutex mutex;
    // 成员默认初始化为 0。普通 int 的 ++ 不是因此自动成为原子操作。
    int value = 0;
};

// local 按值接收 shared_ptr：本函数拥有独立的管理者对象，但仍指向同一个 Counter。
// <> 指定被管理的类型为 Counter；复制管理者不复制 Counter 或其中的 mutex。
void increment_many(std::shared_ptr<Counter> local) {
    // i 是各线程栈帧各自的局部整数；两次调用不会共享同一个 i。
    for (int i = 0; i < 10000; ++i) {
        // -> 沿 local 访问共享 Counter 的 mutex。
        // guard 在本次循环内构造时加锁，另一个线程要等这次 guard 解锁才能进入。
        std::lock_guard<std::mutex> guard(local->mutex);
        // ++ 需要读取、加一、写回；放在同一把锁的保护范围内才不会相互覆盖。
        ++local->value;
        // 到循环体右括号，guard 析构解锁；锁不是整段 10000 次循环一直占着。
    }
    // 这里只把“本函数的 local 管理者”变空，不会把其他 shared_ptr 同时清空。
    // main 的 state、left、right 等拥有者仍在，因此 Counter 此时不会被过早销毁。
    local.reset();
}

int main() {
    // auto 从右侧推导为 shared_ptr<Counter>。make_shared 创建 Counter，value 初始为 0。
    auto state = std::make_shared<Counter>();
    // 三个独立管理者共同指向一个 Counter；复制管理者会新增强拥有关系。
    auto left = state;
    auto right = state;

    // [left] 是 lambda 的按值捕获：闭包保存一份 left 的 shared_ptr 副本。
    // { ... } 是线程要执行的函数体；构造 thread 时就开始安排运行，不是 join 时才启动。
    // 工作线程与 main 之后的语句可能交错，不能由源码书写顺序猜两线程谁先执行。
    std::thread first([left] {
        // 再按值传参复制一份管理者给 local；函数只 reset 它自己的这份。
        increment_many(left);
    });

    try {
        // 第二个线程按值捕获 right。两闭包中的管理者不同，但最终都访问同一份 value。
        std::thread second([right] {
            increment_many(right);
        });
        // join 等待线程完成并建立完成后的同步关系；它不会阻止另一个线程先运行。
        // 即使先 join first，second 也已经启动并可以同时工作。
        first.join();
        second.join();
    } catch (...) {
        // catch (...) 捕获这里传播出的任意异常类型。
        // 这里主要照顾“创建 second 失败，但 first 已启动”的路径：
        // 不能让一个仍 joinable 的 std::thread 对象直接析构，否则程序会 terminate。
        if (first.joinable()) {
            first.join();
        }
        // 收尾后重新抛出原异常，交给上层处理。本例不是通用的线程异常恢复框架；
        // 工作线程函数内部抛出的异常也不会自动传递到 main 的这个 catch。
        throw;
    }

    // 两个 join 已完成，两线程的写入先于此处读取完成；此时没有并发修改 value 的工作线程。
    // 因而这里无需再和已经结束的线程竞争同一把锁。
    std::cout << "value = " << state->value << '\n';
    // main 结束时，已 join 的 thread 可正常析构；管理者逐份销毁，最后释放 Counter。
}
