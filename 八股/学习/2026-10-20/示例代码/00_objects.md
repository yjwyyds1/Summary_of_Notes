---
date: 2026-10-20
updated: 2026-10-21
cpp_standard: C++17
type: 示例代码阅读
tags:
  - 学习/C++/代码阅读
  - 学习/每日八股
---

# 00_objects · 用值、指针和引用读懂对象

← [[学习/2026-10-20/示例代码/README|示例代码阅读目录]] · 相关专题：[[学习/2026-10-20/00-先读懂C++代码与对象|00-先读懂C++代码与对象]] · [[学习/2026-10-20/示例代码/00_objects.cpp|同名 .cpp 源文件]]

## 这份代码用来做什么

把一个整数、保存其地址的指针、绑定到它的引用放在一起比较，再跟着 Quote 的打印观察构造、只读借用、修改和析构。先读懂每个符号，再讨论后续的资源管理。

先看文件开头的总览，再沿步骤读注释。运行方式见 [[学习/2026-10-20/示例代码/README#如何运行|目录中的运行说明]]。下面代码与同名 .cpp 文件逐字一致，包含完整中文注释，可以直接在 Obsidian 阅读视图中看。

## 完整代码

```cpp
// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 本文件先回答“读一行 C++ 时到底在操作谁”：
// 1. int 值、保存地址的指针、给已有对象起别名的引用，是三种不同入口。
// 2. struct 把成员数据和构造/析构行为放在同一种类型中。
// 3. const 引用借用对象来读取；普通引用借用对象来修改，都不接管销毁责任。
// 4. 花括号划出作用域；普通局部对象在离开作用域时自动析构。
// 程序不执行空指针或悬垂指针访问；不要为“看看会怎样”而添加这类访问。

#include <cassert>   // 提供 assert：条件不成立时报告问题并终止，用来检查本例的推导。
#include <iostream>  // 提供 std::cout：把文字和值输出到终端。

// struct 定义一种类型，定义类型本身还没有创建下面 main 中的 Quote 对象。
// struct 的成员默认是 public，因此外部可以写 quote.price。
struct Quote {
    // 每个 Quote 对象都有自己的一份 price，表示这条记录的价格。
    int price;

    // 构造函数与类型同名、没有返回类型，在对象开始使用前完成初始化。
    // explicit 要求明确地构造 Quote，避免把一个 int 默默当成 Quote 传入。
    // int initial 按值接收整数；冒号后的 price(initial) 直接初始化成员。
    explicit Quote(int initial) : price(initial) {
        // << 依次把内容交给 cout；'\n' 是换行字符，price 是本对象的成员。
        std::cout << "construct " << price << '\n';
    }

    // ~Quote 是析构函数：本对象生命周期结束时运行，没有参数和返回值。
    // 这里只打印，方便观察对象结束的时机；不是要自己调用它来清理局部对象。
    ~Quote() {
        std::cout << "destroy " << price << '\n';
    }
};

// void 表示函数不返回结果；const Quote& 表示“借用一个 Quote，只通过此入口读”。
// & 在类型中表示引用，不复制 Quote，也不让函数成为对象的拥有者。
// const 限制通过 quote 修改成员，并不表示其他入口永远不能修改原对象。
void show(const Quote& quote) {
    // 引用像对象名字一样用点号访问成员；此次读取不会触发构造或析构。
    std::cout << "read " << quote.price << '\n';
}

// 普通 Quote& 也借用原对象，但允许修改；函数返回后修改留在调用者的对象里。
void increase(Quote& quote) {
    // 前置 ++ 把原对象的 price 加 1；不是修改一份临时副本。
    ++quote.price;
}

// main 是程序入口；int 是返回状态的类型，走到函数末尾等价于返回 0。
int main() {
    // 步骤 1：创建一个整数对象，再用两种方式访问同一个整数。
    int price = 100;
    // 声明里的 * 表示 pointer 是“指向 int 的指针”；&price 表示取得 price 的地址。
    // pointer 自己也是一个对象，但它保存地址；这里没有复制 price 的整数值。
    int* pointer = &price;
    // 声明里的 & 表示 alias 是 price 的引用：别名必须在这里绑定到已有对象。
    int& alias = price;
    // 表达式里的 * 是解引用：沿 pointer 找到 price，再给 price 赋值为 101。
    *pointer = 101;
    // += 是“在原值上加”；alias 不改绑，修改的仍是 price，结果为 102。
    alias += 1;
    // == 是比较，= 才是赋值。assert 成功时不输出文字。
    // 若用 NDEBUG 关闭断言，这条运行时检查会被移除，不能依赖它实现业务逻辑。
    assert(price == 102);

    // 步骤 2：{100} 用列表初始化创建 Quote，调用其构造函数，先输出 construct 100。
    Quote quote{100};
    // 绑定到 const 引用，读取同一个 Quote，因此输出 read 100。
    show(quote);
    // 绑定到可修改引用，原 quote.price 变成 101；这个函数本身不打印。
    increase(quote);

    // observer 只是借来的观察指针；quote 由 main 的局部作用域管理，不能 delete observer。
    // 它的有效使用期不能超过 quote 的生命周期；非空也不自动等于永远有效。
    Quote* observer = &quote;
    // -> 用于通过指针访问成员；observer->price 与 (*observer).price 表达同一访问。
    assert(observer->price == 101);
    // 再读 quote 会看到修改后的 101，证明 increase 没有只修改一份副本。
    show(quote);

    // 步骤 3：内层花括号建立更短的作用域，temporary_record 只在这里活着。
    {
        Quote temporary_record{200};
        show(temporary_record);
        // 到右花括号自动析构 temporary_record，输出 destroy 200。
    }

    // 此时外层 quote 仍活着；先输出这行，再在 main 结束时输出 destroy 101。
    std::cout << "main ending\n";
}

// 正常运行的完整输出（断言通过时没有额外输出）：
// construct 100
// read 100
// read 101
// construct 200
// read 200
// destroy 200
// main ending
// destroy 101
```

## 怎样观察输出

1. 先在纸上推导 `price`：100 → 101 → 102。这个整数变化没有打印，由断言检查。
2. 对照两次 `show(quote)`：分别是 `read 100`、`read 101`；引用修改的是原对象。
3. 正常完整输出如下。内层对象先销毁，外层 `quote` 在 `main ending` 后才销毁。

```text
construct 100
read 100
read 101
construct 200
read 200
destroy 200
main ending
destroy 101
```

断言成功时不会额外打印；启用 `NDEBUG` 会关闭这些运行时检查。不要通过添加空指针或悬垂访问来验证边界。

以上是预期行为说明；本次注释补充不在这里声称已完成编译或运行，统一记录见 [[学习/2026-10-20/示例验证|示例验证]]。

## 不看注释，自测一下

1. `int* pointer = &price` 中两个符号分别是什么作用？后面的 `*pointer = 101` 又在改谁？
2. `int& alias = price` 是否创建了第二份整数？给 `alias` 赋值会让它改绑吗？
3. `show(const Quote&)` 与 `increase(Quote&)` 为什么一个能读、一个能改？有没有复制 Quote？
4. 为什么不能 `delete observer`？它的有效期由什么决定？
5. 为什么 `destroy 200` 在 `main ending` 之前，而 `destroy 101` 在其后？

> [!tip]- 展开核对关键答案
> 1. 声明里的 `*` 组成指针类型，`&price` 取地址；表达式里的 `*pointer` 解引用，修改原来的 `price`。
> 2. 没有第二份整数。引用是访问同一对象的别名，赋值修改被引用对象，不改绑。
> 3. 两者都借用原 Quote。`const` 限制通过该入口修改；普通引用允许修改，不涉及复制构造。
> 4. observer 只借用 main 内局部 Quote 的地址，没有取得删除责任。它不能比所观察对象活得更久还继续访问。
> 5. 内层块先结束，temporary_record 先析构；外层 quote 到 main 的作用域结束才析构。
