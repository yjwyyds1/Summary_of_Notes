---
date: 2026-10-20
updated: 2026-10-21
cpp_standard: C++17
type: 示例代码阅读
tags:
  - 学习/C++/代码阅读
  - 学习/每日八股
---

# 02_raii · 沿返回和异常路径观察 RAII

← [[学习/2026-10-20/示例代码/README|示例代码阅读目录]] · 相关专题：[[学习/2026-10-20/02-RAII与异常清理|02-RAII与异常清理]] · [[学习/2026-10-20/示例代码/02_raii.cpp|同名 .cpp 源文件]]

## 这份代码用来做什么

观察锁守卫、动态数组和打印对象如何随作用域结束清理，再检查构造失败时哪些析构会运行。把资源自动归还与业务修改自动回滚明确区分。

先看文件开头的总览，再沿步骤读注释。运行方式见 [[学习/2026-10-20/示例代码/README#如何运行|目录中的运行说明]]。下面代码与同名 .cpp 文件逐字一致，包含完整中文注释，可以直接在 Obsidian 阅读视图中看。

## 完整代码

```cpp
// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// RAII 的读法：让一个对象负责一项资源，其析构函数负责归还资源。
// 本例用 lock_guard 管锁、unique_ptr 管数组，观察普通返回和异常离开时的清理。
// 还要区分：自动归还资源，并不等于自动撤销所有已经完成的业务修改。
// 三种 mode 都先把 101 存入 saved，所以 mode 2 抛异常后那次保存仍然存在。
// Packet 部分演示：构造失败时，已构造成功的成员会清理，整个 Packet 的析构体不运行。

#include <cassert>    // assert 检查保存数量；成功时没有输出，NDEBUG 可关闭检查。
#include <cstdio>     // printf/puts/fprintf 输出执行顺序；stderr 用于意外错误信息。
#include <exception>  // std::exception：最外层捕获标准异常时使用的基类。
#include <memory>     // make_unique、unique_ptr：为动态数组建立自动清理责任。
#include <mutex>      // mutex、lock_guard：互斥锁本体以及负责加锁/解锁的局部守卫。
#include <stdexcept>  // runtime_error：本例主动抛出并捕获的异常类型。
#include <vector>     // vector<int>：保存已完成的结果，观察异常不会自动回滚它。

// Part 只打印生命周期，用它看清“谁已经构造成功，谁需要被析构”。
class Part {
public:
    // const char* 借用字符串字面量；fail 的默认实参是 false，因此可只传 name。
    // explicit 禁止把一个 name 参数隐式变成 Part；name_(name) 先初始化成员指针。
    explicit Part(const char* name, bool fail = false) : name_(name) {
        std::printf("start %s\n", name_);
        // if 的条件为 true 时执行块；throw 构造并抛出异常，正常控制流在此中断。
        if (fail) {
            throw std::runtime_error("part construction failed");
        }
        // 只有未抛异常才能走到 ready：该 Part 的构造才能成功完成。
        std::printf("ready %s\n", name_);
    }

    // noexcept 声明析构不让异常向外传播；析构负责清理，不应在异常清理途中再抛异常。
    ~Part() noexcept {
        std::printf("destroy %s\n", name_);
    }

private:
    // 只保存名字的位置，不拥有字符存储；这里的字面量一直有效，不需要 delete。
    const char* name_;
};

struct Packet {
    // 成员按“声明顺序”初始化：先 first 后 second；成功后按逆序销毁。
    Part first;
    Part second;

    // bool fail 按值传入，转交给 second 的构造函数。
    // 冒号后的成员初始化发生在 Packet 构造函数体之前。
    explicit Packet(bool fail)
        : first("member-first"), second("member-second", fail) {
        // 若 second 抛异常，根本不会进入这里，也不会执行 ~Packet 的函数体。
        std::puts("Packet constructor body");
    }

    // 仅对成功构造出的 Packet 运行：先执行析构体，再自动析构 second、first。
    ~Packet() noexcept {
        std::puts("Packet destructor body");
    }
};

// int 是返回值类型；两个 & 参数都借用 main 的对象，不复制 mutex/vector。
// saved 是可修改引用，因此 push_back 的结果能被 main 看见；mode 只是整数副本。
int process(std::mutex& mutex, std::vector<int>& saved, int mode) {
    // <> 把 lock_guard 模板的锁类型指定为 std::mutex。
    // 构造 lock 时加锁；lock 活着就持锁，离开作用域析构时解锁。
    // 必须保证被引用的 mutex 比这个守卫活得更久，本例由 main 满足。
    std::lock_guard<std::mutex> lock(mutex);
    // 以下局部对象按顺序成功构造；清理时按相反顺序处理。
    Part first("scope-first");
    Part second("scope-second");
    // auto 推导为 unique_ptr<int[]>；int[] 是数组形式，拥有者析构时匹配数组删除。
    // make_unique<int[]>(3) 创建三个 int，buffer 用下标访问已存在的数组元素。
    auto buffer = std::make_unique<int[]>(3);
    buffer[0] = 101;
    // 复制这个整数到调用者的 vector；一旦成功，后面的异常不会自动删除这条记录。
    saved.push_back(buffer[0]);

    if (mode == 1) {
        // 提前 return 仍会清理 buffer、second、first、lock，然后把 -1 返回调用者。
        return -1;
    }
    if (mode == 2) {
        // 异常沿调用栈寻找 catch，途中清理本作用域已成功构造的对象，称为栈展开。
        // 这里仍会释放数组、销毁两个 Part、解锁，但 saved 不是本函数的局部副本。
        throw std::runtime_error("processing failed after saving");
    }
    // mode 0：back() 读取最后一个现有元素，先取得返回的整数，再清理局部对象。
    // 前面的 push_back 已成功，所以这里不是对空容器取 back。
    return saved.back();
}

int main() {
    // 外层 try 处理演示之外的标准异常，例如分配失败；不是承诺任何错误都可恢复。
    try {
        // mutex 本身与“当前持有它的 lock_guard”是两个对象。
        std::mutex mutex;
        std::vector<int> saved;

        // for 的三部分：mode 从 0 开始；不是 3 就继续；每轮结束 ++mode。
        // 因而依次运行 0、1、2 三种退出路径。
        for (int mode = 0; mode != 3; ++mode) {
            std::printf("mode %d\n", mode);
            try {
                // const 禁止随后修改这次得到的 result；异常时这条初始化不会完成。
                const int result = process(mutex, saved, mode);
                // mode 0 输出 result 101，mode 1 输出 result -1；mode 2 跳过此行。
                std::printf("result %d\n", result);
            } catch (const std::runtime_error& error) {
                // const & 只借用异常对象，不复制、不修改；what() 提供解释文字。
                std::printf("caught: %s\n", error.what());
            }
            // 再次取得同一把锁，检查 process 的退出路径确实没有把锁一直占住。
            // check 的作用域到本轮末尾；下一轮开始前它会析构解锁。
            std::lock_guard<std::mutex> check(mutex);
            // size() 是无符号大小类型；static_cast 明确把 mode + 1 转成该类型比较。
            // 这检查的是“已保存的修改没有被自动回滚”：每轮都多一条。
            assert(saved.size() == static_cast<std::size_t>(mode + 1));
        }

        // %zu 对应 size_t，三种 mode 运行后正常应输出 saved count = 3。
        std::printf("saved count = %zu\n", saved.size());
        std::puts("construction failure:");
        try {
            // first 成功，second 打印 start 后抛异常：只清理已完成构造的 first。
            // second 自己的 ~Part 和 failed 的 ~Packet 均不会在此运行。
            Packet failed(true);
        } catch (const std::runtime_error& error) {
            std::printf("caught: %s\n", error.what());
        }

        std::puts("successful construction:");
        {
            // 这次两个成员都 ready，进入构造体；到右括号按完整对象的规则析构。
            Packet complete(false);
        }
    } catch (const std::exception& error) {
        // 意外标准异常走这条路径，用错误输出报告并返回非零状态。
        std::fprintf(stderr, "unexpected failure: %s\n", error.what());
        return 1;
    }
}

// 观察重点：每个 mode 都有 destroy scope-second，再有 destroy scope-first。
// mode 2 的 caught 提示出现在这两次析构之后，保存数量最后仍为 3。
// 构造失败段没有 Packet destructor body，也没有 destroy member-second；
// 成功段则依次有 Packet destructor body、destroy member-second、destroy member-first。
// RAII 保证资源与对象作用域绑定；“业务要么全成功要么不改”的事务语义需另外设计。
```

## 怎样观察输出

1. mode 0、1、2 都先构造 `scope-first`、`scope-second`；三条退出路径都会先 `destroy scope-second`，再 `destroy scope-first`。
2. mode 0 输出 `result 101`，mode 1 输出 `result -1`；mode 2 在局部清理后输出 `caught: processing failed after saving`。
3. 三次都已完成一次保存，所以最后是 `saved count = 3`。RAII 没有替程序撤销 vector 中的修改。
4. 构造失败段没有 `Packet constructor body`、`Packet destructor body` 或 `destroy member-second`，但有 `destroy member-first`。
5. 成功段先有 `Packet destructor body`，随后 `destroy member-second`、`destroy member-first`。

unexpected failure 分支是意外标准异常的报告路径。断言与打印用于本例观察，不是对所有异常处理设计的完整证明。

以上是预期行为说明；本次注释补充不在这里声称已完成编译或运行，统一记录见 [[学习/2026-10-20/示例验证|示例验证]]。

## 不看注释，自测一下

1. `process` 为什么用引用接收 mutex 和 saved？如果只看返回值，会漏掉什么效果？
2. `return -1` 与 `throw` 都离开 process，buffer、两个 Part 和锁按什么顺序清理？
3. mode 2 抛异常后 saved 为什么没有恢复成调用前的大小？
4. second 构造失败时，为什么 first 的析构运行，而 second 和 Packet 的析构体不运行？
5. `const std::runtime_error& error` 中 const 与 & 的用处是什么？noexcept 是否代表编译器证明所有调用都不会失败？

> [!tip]- 展开核对关键答案
> 1. 借用同一把锁和调用者的保存容器，不复制；saved 的追加修改也是函数的效果，不仅是返回一个 int。
> 2. 对已成功构造的局部对象，先 buffer，再 second、first，最后 lock；lock 析构负责解锁。
> 3. saved 是外部容器，追加已经成功。资源清理不自动撤销已完成的业务修改，回滚需要另外设计。
> 4. first 已完成构造，需要清理；second 与整个 Packet 尚未构造成功，所以不运行它们各自完整对象的析构体。
> 5. 引用避免复制异常对象，const 防止通过该引用修改；noexcept 是不让异常传播的声明，不是编译器已经证明代码绝不失败。
