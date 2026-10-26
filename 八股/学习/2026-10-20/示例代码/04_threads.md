---
date: 2026-10-20
updated: 2026-10-21
cpp_standard: C++17
type: 示例代码阅读
tags:
  - 学习/C++/代码阅读
  - 学习/每日八股
---

# 04_threads · 给共享对象的数据正确加锁

← [[学习/2026-10-20/示例代码/README|示例代码阅读目录]] · 相关专题：[[学习/2026-10-20/04-shared_ptr与线程安全|04-shared_ptr与线程安全]] · [[学习/2026-10-20/示例代码/04_threads.cpp|同名 .cpp 源文件]]

## 这份代码用来做什么

两个线程各持 shared_ptr 副本，共同修改一个 Counter。沿捕获、按值传参、加锁、自增、join 的顺序理解：生命周期管理与数据访问同步分别由谁完成。

先看文件开头的总览，再沿步骤读注释。运行方式见 [[学习/2026-10-20/示例代码/README#如何运行|目录中的运行说明]]。下面代码与同名 .cpp 文件逐字一致，包含完整中文注释，可以直接在 Obsidian 阅读视图中看。

## 完整代码

```cpp
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
```

## 怎样观察输出

1. 正常成功路径只输出 `value = 20000`；每个线程执行 10000 次受同一把锁保护的自增。
2. first 与 second 的创建语句先后书写，不代表它们按同样顺序完成；构造 thread 就开始安排运行。
3. 两次 join 完成后 main 才读 value。先 join first 不会把已经启动的 second 挡到 join 之后才运行。
4. local.reset 只清空本函数收到的管理者副本；main 的 state 和闭包中的拥有者不会随之同时清空。

这是互斥锁示例，不能用一次正确输出证明任意改写版本都没有数据竞争。工作线程内部异常不会自动进入 main 中的 catch。

以上是预期行为说明；本次注释补充不在这里声称已完成编译或运行，统一记录见 [[学习/2026-10-20/示例验证|示例验证]]。

## 不看注释，自测一下

1. `[left]` 按值捕获复制的是 Counter 还是 shared_ptr？两线程最终修改几份 value？
2. 为什么引用计数可以协调，不代表普通 int 的 ++ 就可去掉锁？
3. guard 的有效期是一轮循环还是整个 increment_many 调用？这决定锁什么时候释放？
4. local.reset 会不会让 state 也变空？Counter 此时为什么仍然活着？
5. join 在做什么？为什么 main 读取 value 时不用再和这两个已结束线程争锁？

> [!tip]- 展开核对关键答案
> 1. 复制 shared_ptr 管理者；各自拥有者仍指向同一个 Counter，所以只修改一份共享 value。
> 2. 管理者的引用计数与被指对象的数据是两层。value 是普通 int，读改写需要同一把 mutex 协调。
> 3. guard 在循环体内构造，到该轮右括号析构解锁；每轮重新竞争锁。
> 4. reset 只改 local，其他独立管理者不受此操作直接改变；它们仍持有强拥有关系。
> 5. join 等待线程完成，并建立完成后的同步关系。两线程都结束后不再并发写 value，main 随后读取。
