---
date: 2026-10-20
updated: 2026-10-21
cpp_standard: C++17
type: 示例代码阅读
tags:
  - 学习/C++/代码阅读
  - 学习/每日八股
---

# 03_ownership · 追踪独占、共享与弱观察

← [[学习/2026-10-20/示例代码/README|示例代码阅读目录]] · 相关专题：[[学习/2026-10-20/03-所有权与智能指针|03-所有权与智能指针]] · [[学习/2026-10-20/示例代码/03_ownership.cpp|同名 .cpp 源文件]]

## 这份代码用来做什么

按输出找到每个 Quote 真正被销毁的位置，区分借用地址与转交所有权；最后用父子节点理解 weak_ptr 为什么不增加强拥有关系。

先看文件开头的总览，再沿步骤读注释。运行方式见 [[学习/2026-10-20/示例代码/README#如何运行|目录中的运行说明]]。下面代码与同名 .cpp 文件逐字一致，包含完整中文注释，可以直接在 Obsidian 阅读视图中看。

## 完整代码

```cpp
// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 沿着“谁负责最后销毁对象”读本例，而不只看谁保存了地址。
// 1. 引用和裸指针用于借用，不自动取得所有权。
// 2. unique_ptr 的移动转交独占所有权；release 放弃管理但不销毁；reset 释放并变空。
// 3. shared_ptr 允许多个强拥有者；weak_ptr 只观察，先 lock 成功才访问对象。
// 4. 树节点的父方向用 weak_ptr，避免父子用强拥有关系互相留住。
// 这是单线程示例：共享所有权本身不能推出被管理对象可被多线程随意读写。

#include <iostream>  // cout 输出生命周期；boolalpha 把布尔值显示为 true/false。
#include <memory>    // unique_ptr/shared_ptr/weak_ptr 及其 make_* 创建函数。
#include <string>    // string 拥有并管理 symbol/name 的字符内容。
#include <utility>   // std::move 把表达式转成可供移动操作选择的形式，本身不搬运资源。

struct Quote {
    // 每个 Quote 拥有自己的字符串成员和整数成员；字符串自己管理内部字符存储。
    std::string symbol;
    int price;

    // name 按值接收一个字符串；std::move(name) 让 symbol 的初始化可以使用移动构造。
    // 真正执行转移的是 string 的构造函数，不是 std::move 这次转换本身。
    // price(value) 初始化整数成员；成员按声明顺序初始化。
    Quote(std::string name, int value)
        : symbol(std::move(name)), price(value) {
        std::cout << "create " << symbol << '\n';
    }

    // 最后一个拥有者释放 Quote 时能看到此行；析构体之后还会自动析构 string 成员。
    ~Quote() {
        std::cout << "destroy " << symbol << '\n';
    }
};

// const Quote& 表示只读借用：必须有一个有效对象，不复制 Quote，也不接管销毁责任。
void print_quote(const Quote& quote) {
    // 点号通过对象或引用访问成员；<< 连续输出代码和值，'\n' 表示换行。
    std::cout << quote.symbol << " " << quote.price << '\n';
}

// const Quote* 是指向只读 Quote 的指针；允许调用者用 nullptr 表达“当前没有对象”。
// const 限制通过此参数改对象；指针变量本身按值传入，函数并不拥有该对象。
void print_if_present(const Quote* quote) {
    // != 表示不等于；先判断，再解引用。非空仍需调用者保证对象尚在生命周期中。
    if (quote != nullptr) {
        // * 在表达式中是解引用，得到所指对象，再绑定给 print_quote 的 const 引用。
        print_quote(*quote);
    }
}

// unique_ptr 按值接收：调用者必须转交独占所有权，不能复制 unique_ptr。
// 本例传入的 quote 非空；此函数没有处理空 unique_ptr，不能任意传空后仍解引用。
void consume(std::unique_ptr<Quote> quote) {
    print_quote(*quote);
} // 参数拥有的 Quote 在本次调用结束时由 unique_ptr 清理，输出 destroy AAA。

struct Node {
    std::string name;
    // child 是强拥有关系：父节点活着并保存此成员时，子节点被它拥有。
    // <Node> 是模板实参，把智能指针的被管理类型指定为 Node。
    std::shared_ptr<Node> child;
    // parent 是弱观察关系：知道父节点是谁，但不把父节点的强拥有计数加一。
    std::weak_ptr<Node> parent;

    // explicit 防止单个 string 隐式转成 Node；这里只把 text 的内容交给 name。
    explicit Node(std::string text) : name(std::move(text)) {}

    ~Node() {
        std::cout << "destroy node " << name << '\n';
        // 函数体结束后，parent/child/name 也按成员逆序析构；child 会释放它的强拥有份额。
    }
};

int main() {
    std::cout << "[unique ownership]\n";
    // auto 从右侧推导为 unique_ptr<Quote>；make_unique 分配并构造一个 Quote。
    auto owner = std::make_unique<Quote>("AAA", 100);
    print_quote(*owner);                  // 解引用后借给 const 引用，不新增拥有者。
    print_if_present(owner.get());        // get() 借出地址，owner 仍负责销毁。
    auto next = std::move(owner);         // 创建 next 时移动 unique_ptr，转交独占所有权。
    // owner 已变空；next 仍拥有原来的 Quote。boolalpha 让比较结果打印为 true。
    std::cout << "owner empty: " << std::boolalpha
              << (owner == nullptr) << '\n';
    consume(std::move(next));             // 参数接管所有权；返回后 next 为空，AAA 已销毁。

    std::cout << "[release and reset]\n";
    auto source = std::make_unique<Quote>("BBB", 200);
    // release() 返回原始指针，并让 source 变空；它没有 delete，BBB 此刻仍活着。
    // 放弃后必须安排新的清理责任，否则会泄漏。这里立刻交给另一个 unique_ptr。
    Quote* raw = source.release();
    std::unique_ptr<Quote> rescued(raw);   // 用裸指针接管同一对象，只能有一个这样的独占拥有者。
    rescued.reset();                      // 立即删除 BBB 并变空；raw 之后不能再解引用或删除。

    std::cout << "[shared ownership]\n";
    // 默认构造的 weak_ptr 为空；它在外层存活，稍后用来观察内层对象是否还活着。
    std::weak_ptr<Quote> observer;
    {
        auto first = std::make_shared<Quote>("CCC", 300);
        observer = first;                // 记录弱观察关系，不延长 Quote 的生命周期。
        {
            auto second = first;         // 复制 shared_ptr，新增一份强拥有关系，不复制 Quote。
            first.reset();               // 只释放 first 这份拥有关系；second 仍让对象活着。
            // if 中声明的 auto temporary 是 shared_ptr<Quote>；作用域只到这个 if 块结束。
            // lock() 尝试取得一份强拥有关系；成功时条件为真，期间对象不会被最后释放。
            if (auto temporary = observer.lock()) {
                print_quote(*temporary);
            }                            // temporary 释放自己的份额；此时 second 仍拥有对象。
        }                                // second 释放最后一份强拥有关系，CCC 在这里析构。
        // boolalpha 仍生效，输出 expired: true；弱观察者本身还在，不等于对象还在。
        std::cout << "expired: " << observer.expired() << '\n';
    }
    // 不要先 expired() 再假定以后一定有效；需要使用时直接检查 lock() 返回的拥有者。
    if (auto temporary = observer.lock()) {
        print_quote(*temporary);
    } else {
        std::cout << "Quote no longer alive\n";
    }
    // 解除弱观察关系；Quote 已经销毁，这不是在第二次删除 Quote。
    observer.reset();

    std::cout << "[weak back edge]\n";
    {
        auto root = std::make_shared<Node>("root");
        // -> 通过智能指针访问 Node 的成员；root 的 child 成员强拥有新 leaf。
        root->child = std::make_shared<Node>("leaf");
        root->child->parent = root;       // leaf 只弱观察 root，避免形成强拥有的环。
        if (auto parent = root->child->parent.lock()) {
            std::cout << "parent: " << parent->name << '\n';
        }
    }                                    // 最后先打印 destroy node root，再打印 destroy node leaf。
}

// 观察重点：AAA 在 consume 返回时销毁；BBB 在 reset 时销毁；CCC 在 second 离开时销毁。
// 空拥有者的后续析构不会重复删除；不要从 get() 再造一个独立的拥有者去管理同一对象。
// 本例假定分配成功；移动 unique_ptr 不会让已有的裸观察指针自动失效或自动延寿，
// 判断观察指针能否使用，仍要看它所指对象是否活着。
```

## 怎样观察输出

1. AAA 创建后输出两次 `AAA 100`，转交给 consume 时再输出一次；`owner empty: true` 后，AAA 在 consume 返回时销毁。
2. BBB 的 release 不打印 destroy，随后的 rescued.reset 才打印 `destroy BBB`。
3. CCC 在 `second` 离开内层作用域时销毁，之后输出 `expired: true` 与 `Quote no longer alive`。
4. 最后打印 `parent: root`；该小块退出时依次打印 `destroy node root`、`destroy node leaf`。

`std::boolalpha` 一旦设置，后续布尔输出也显示 true/false。本例只在传入对象有效、分配成功的路径下演示，不给 consume 传空指针再解引用。

以上是预期行为说明；本次注释补充不在这里声称已完成编译或运行，统一记录见 [[学习/2026-10-20/示例验证|示例验证]]。

## 不看注释，自测一下

1. `print_quote(*owner)` 与 `print_if_present(owner.get())` 是否新增 Quote 或新的拥有者？
2. `std::move(owner)` 与 `owner.release()` 都可能让 owner 变空，清理责任有什么差别？
3. rescued.reset 后 raw 是否自动变成 nullptr？还能 delete raw 吗？
4. weak_ptr 为什么需要 lock？lock 得到的 temporary 在读取期间提供了什么保证？
5. 如果 Node::parent 也改为 shared_ptr，父子关系为什么可能使两个节点互相留住？

> [!tip]- 展开核对关键答案
> 1. 都只是借用同一个 Quote；解引用与 get 本身不复制 Quote，也不新增强拥有者。
> 2. 此处移动构造 next 会把责任直接转交给 next；release 只放弃管理并返回裸指针，调用者必须安排新的清理责任。
> 3. raw 不会自动置空，原对象已销毁；既不能再读，也不能再次 delete。
> 4. lock 尝试取得 shared_ptr 强拥有者。成功后 temporary 在其有效期内保持对象活着，不能仅凭早先 expired 的检查就访问。
> 5. 两方向都强拥有会形成环；只丢掉外部 root 不一定把强拥有数降到零。这里用 weak_ptr 表示回指，避免那条强拥有边。
