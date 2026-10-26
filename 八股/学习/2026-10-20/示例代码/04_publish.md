---
date: 2026-10-20
updated: 2026-10-21
cpp_standard: C++17
type: 示例代码阅读
tags:
  - 学习/C++/代码阅读
  - 学习/每日八股
---

# 04_publish · 原子发布完整的只读快照

← [[学习/2026-10-20/示例代码/README|示例代码阅读目录]] · 相关专题：[[学习/2026-10-20/04-shared_ptr与线程安全|04-shared_ptr与线程安全]] · [[学习/2026-10-20/示例代码/04_publish.cpp|同名 .cpp 源文件]]

## 这份代码用来做什么

写线程先做好 Snapshot，再原子替换共享指针槽位；读线程取得自己的 shared_ptr 副本，用同一个快照读取一对字段。用 C++17 接口读懂发布与生命周期的关系。

先看文件开头的总览，再沿步骤读注释。运行方式见 [[学习/2026-10-20/示例代码/README#如何运行|目录中的运行说明]]。下面代码与同名 .cpp 文件逐字一致，包含完整中文注释，可以直接在 Obsidian 阅读视图中看。

## 完整代码

```cpp
// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 场景：写线程做好一份新配置快照，再让读线程原子地取得一份完整的快照拥有者。
// 与 04_threads.cpp 的区别：那里保护“对象里的可变计数”，这里保护“共享指针槽位”。
// current 是两个线程共用的同一个 shared_ptr 对象；并发读写它统一用 C++17 的
// shared_ptr 专用 atomic_load/atomic_store 自由函数，不能混用普通赋值和普通读取。
// Snapshot 经 shared_ptr<const Snapshot> 发布，读者通过该接口只读，不再原地修改快照。
// 这些原子接口不承诺实现一定无锁；本例不做延迟或性能结论。

#include <atomic>    // 提供原子操作相关工具；本例采用 shared_ptr 的原子访问接口。
#include <iostream>  // cout 输出读线程看到的版本和值，以及最后发布完成的快照。
#include <memory>    // shared_ptr/make_shared 及 C++17 针对 shared_ptr 的 atomic_load/store 重载。
#include <thread>    // thread 启动线程；join 等待完成后再读取线程写下的普通整数结果。

// 快照把相关字段放进同一个对象，避免把两个字段分别发布后拼成不一致的一对。
struct Snapshot {
    int version;
    int limit;
};

int main() {
    // const 修饰 Snapshot：通过这个指针不能修改其字段；current 管理者本身仍可换指向。
    // Snapshot{0, 100} 按成员声明顺序初始化 version、limit。
    // main 在线程启动前建立初始快照，此时不与其他线程并发访问 current。
    std::shared_ptr<const Snapshot> current =
        std::make_shared<const Snapshot>(Snapshot{0, 100});
    // -1 是尚未记录的哨兵值；这两个普通 int 只有 reader 写，main 在 reader.join 后读。
    // 它们无需仅因“涉及线程”就变成 atomic，关键是实际读写有没有同步地安排好。
    int observed_version = -1;
    int observed_limit = -1;

    // [&current] 按引用捕获：闭包使用 main 中同一个 current 槽位，没有复制管理者。
    // 被引用的局部变量必须活到线程结束；后面的 join 使本例满足这个要求。
    // thread 构造时就启动工作，lambda 的大括号是工作线程执行的函数体。
    std::thread writer([&current] {
        // auto 推导为 shared_ptr<const Snapshot>；先完整构造新对象，此时只由写线程准备。
        auto next = std::make_shared<const Snapshot>(Snapshot{1, 200});
        // &current 在表达式中是取地址，让函数知道要原子更新哪个 shared_ptr 槽位。
        // store 发布 next 的一份共享拥有关系；next 稍后析构也不妨碍 current 继续拥有快照。
        // 默认的这组接口使用顺序一致原子操作；读者若取得新快照，也能看到发布前的初始化。
        std::atomic_store(&current, next);
    });

    try {
        // 三个 & 捕获都借用 main 的变量；不是把 -1 或初始 current 拷贝进线程。
        // 读写线程启动后谁先完成没有固定保证，reader 可能读到旧快照，也可能读到新快照。
        std::thread reader([&current, &observed_version, &observed_limit] {
            // load 原子地取得一个 shared_ptr 副本，local 是读线程自己的强拥有者。
            // 即便 current 随后换指向，local 仍保持它取得的那一份快照活着。
            auto local = std::atomic_load(&current);
            // -> 沿同一个 local 读取两字段，所以得到 0:100 或 1:200，不会混成 0:200。
            observed_version = local->version;
            observed_limit = local->limit;
            // local 在 lambda 末尾析构，释放自己的强拥有份额。
        });
        // join 是等待，不是启动。先等 writer 不代表 reader 一定在 writer 之后才读取。
        writer.join();
        reader.join();
    } catch (...) {
        // 主要覆盖 reader 的线程创建失败而 writer 已启动的路径，先收尾已有线程。
        // joinable 表示该 thread 还关联着需 join/detach 的线程，不等于它此刻一定仍在运行。
        if (writer.joinable()) {
            writer.join();
        }
        // 重新抛出原异常；不是把工作线程内部的任意异常都转发回 main。
        throw;
    }

    // reader 已 join，故 main 现在可以读取它记录的两个普通 int，不与 reader 并发。
    std::cout << "reader saw " << observed_version
              << ":" << observed_limit << '\n';
    // writer 已 join，最后的 store 已完成；继续统一使用 atomic_load 读取 current。
    auto final_snapshot = std::atomic_load(&current);
    std::cout << "final " << final_snapshot->version
              << ":" << final_snapshot->limit << '\n';
}

// 正常成功路径的两种允许输出：
// reader saw 0:100          或 reader saw 1:200
// final 1:200                  final 1:200
// 不要靠反复运行“总看见同一种结果”来推断线程一定按某个固定顺序执行。
// 这段代码没有用 C++20 的 atomic<shared_ptr<T>>，也没有演示可变快照内容的并发写入。
```

## 怎样观察输出

1. 允许的第一行是 `reader saw 0:100` 或 `reader saw 1:200`，取决于读线程在更新前还是更新后取得快照。
2. 两种情况下第二行都是 `final 1:200`，因为这次读取在 writer.join 完成后。
3. 不应出现 `0:200` 或 `1:100`：读线程的两个字段都来自同一个 local 所拥有的不可变快照。
4. 反复运行总看见一种情况，也不能证明线程有固定先后顺序；join 的书写顺序不规定较早的读取发生在何时。

这里用 C++17 的 shared_ptr 专用 atomic_load/store；没有使用 C++20 的 atomic<shared_ptr<T>>，也不声称底层实现一定无锁。正常输出分析假定线程创建和分配成功。

以上是预期行为说明；本次注释补充不在这里声称已完成编译或运行，统一记录见 [[学习/2026-10-20/示例验证|示例验证]]。

## 不看注释，自测一下

1. `shared_ptr<const Snapshot>` 中 const 限制的是快照内容，还是 current 能否换指向？
2. `[&current]` 为什么必须考虑 current 的有效期？这里由哪一步保证线程结束前 main 不离开？
3. atomic_load 返回的 local 为什么能让旧快照在 current 换指向后继续活着？
4. 为什么不能把 writer 的 atomic_store 改成普通 `current = next`，而 reader 仍原子读取同一个槽位？
5. observed_version/limit 为什么仍可用普通 int？先 join writer 是否迫使 reader 读到新版本？

> [!tip]- 展开核对关键答案
> 1. const 限制通过该指针修改 Snapshot；current 这个管理者仍可被原子替换为指向另一份快照。
> 2. 引用捕获借用 main 的变量，不能在线程还使用它时销毁；本例在离开作用域前 join 已启动的线程。
> 3. local 是读线程自己的强拥有者，不是对 current 的别名；它持有的快照不会仅因 current 替换就销毁。
> 4. 同一个 shared_ptr 槽位的并发读写需要统一的安全访问方式；混入普通赋值不能因另一侧使用原子接口就自动安全。
> 5. reader 独占写入，main 在 reader.join 后读取。reader 已在两个 join 前启动，可能早在 writer 更新前就读到旧版本。
