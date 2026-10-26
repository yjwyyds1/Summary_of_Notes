---
date: 2026-10-21
id: CPP-VIRTUAL-DTOR-001
standard: C++17
status: 未自测
tags: [C++, 示例代码, 多态]
---

# 05_virtual_destructor · 基类拥有与完整析构

入口：[[学习/学习导航/2026-10-21-学习导航|当天学习入口]]  
对应正文：[[学习/2026-10-21/05-虚析构与基类销毁|专题详解]]  
源码：[[学习/2026-10-21/示例代码/05_virtual_destructor.cpp|05_virtual_destructor.cpp]]  
自测：[[八股测试/2026-10-21|当天测试页]]

## 用途与阅读前提

用 unique_ptr<Channel> 拥有一个真实 FileChannel，通过日志和外部计数器观察完整析构。只包含正确删除路径，不执行非虚基类删除派生对象的 UB。

程序是独立 C++17 翻译单元，单独编译，不与其他带 main 的示例拼接。
本页完整代码与同名 cpp、专题正文中的对应完整代码保持一致。
这里只列规则推导的预期输出；实际编译运行记录见当天统一验证结果。
不运行错误删除或其他未定义行为来猜固定输出。

## 完整中文注释源码

```cpp
// C++17：虚析构保证 unique_ptr<Channel> 能完整销毁真实派生对象。
// 所有可执行路径都安全；非虚析构的错误设计仅在配套 Markdown 中静态分析。

#include <cassert>   // 提供 assert：核对析构前后的计数。
#include <iostream>  // 提供 std::cout：观察构造、业务与析构顺序。
#include <memory>    // 提供 std::unique_ptr 和 std::make_unique。
#include <string>    // 提供 std::string：保存通道名称。

// 一个小计数器：在拥有者销毁后仍能检查两个析构函数是否执行。
struct LifetimeCount {
    int derived = 0;  // 派生类析构函数体执行次数，起始为 0。
    int base = 0;     // 基类析构函数体执行次数，起始为 0。
};

// Channel 是抽象通信接口：调用者可使用，也可通过基类拥有并销毁它。
class Channel {
public:
    // 接收一个已存在计数器的引用；本类不拥有该计数器。
    explicit Channel(LifetimeCount& counts) : counts_(counts) {
        std::cout << "Channel ctor\n";
    }

    // public 允许外部删除；virtual 使删除遵循真实派生类型。
    // 析构函数不能写返回类型；它不是普通的 void 成员函数。
    virtual ~Channel() {
        ++counts_.base;  // ++ 把计数加一，记录基类析构函数体执行。
        std::cout << "Channel dtor\n";
    }

    // = 0 让 Channel 保持抽象；const 表示调用不修改普通成员。
    virtual int send() const = 0;

private:
    LifetimeCount& counts_;  // 非拥有引用；main 保证计数器活得更久。
};

// FileChannel 是一个具体通道；final 表示不再继续派生。
class FileChannel final : public Channel {
public:
    // 两个形参分别提供计数器和名称；名称以 const 引用避免形参复制。
    FileChannel(LifetimeCount& counts, const std::string& name)
        : Channel(counts), name_(name), counts_(counts) {
        // 先构造基类，再按声明顺序初始化 name_、counts_，最后进入此处。
        std::cout << "FileChannel ctor\n";
    }

    // 基类析构已经 virtual，因此派生析构可用 override 核对覆盖关系。
    ~FileChannel() override {
        ++counts_.derived;  // 只记录派生析构函数体，不手工调用基类析构。
        std::cout << "FileChannel dtor\n";
        // 函数体结束后自动销毁本类成员，再自动销毁 Channel 基类子对象。
    }

    // override 核对 send 的返回类型、参数和 const 都与基类要求兼容。
    int send() const override {
        std::cout << "send: " << name_ << '\n';
        return 7;  // 教学用固定返回值，不进行真实文件或网络操作。
    }

private:
    std::string name_;       // 拥有名称文本；析构会自动清理其内部资源。
    LifetimeCount& counts_;  // 借用同一个计数器，供派生析构记录。
};

int main() {  // 程序入口；以下花括号用来明确拥有者的销毁时刻。
    LifetimeCount counts;  // 在内层作用域之前创建，在其之后才销毁。

    {
        // make_unique<FileChannel> 创建真实 FileChannel。
        // 转为 unique_ptr<Channel> 后仍是同一对象，并未复制成 Channel。
        std::unique_ptr<Channel> channel =
            std::make_unique<FileChannel>(counts, "quotes");

        // -> 通过智能指针访问对象；虚调用进入 FileChannel::send。
        const int result = channel->send();
        assert(result == 7);
        assert(counts.derived == 0 && counts.base == 0);

        std::cout << "before owner leaves\n";
    }  // channel 析构：默认删除器经 Channel* 删除，虚析构选择完整链条。

    // 内层拥有者已经退出，外层计数器还活着，可以安全读取。
    assert(counts.derived == 1);
    assert(counts.base == 1);
    std::cout << "after owner leaves\n";
    return 0;
}
```

## 预期输出

```text
Channel ctor
FileChannel ctor
send: quotes
before owner leaves
FileChannel dtor
Channel dtor
after owner leaves
```

## 输出理解

先构造基类部分，再构造派生部分，所以两条 ctor 日志按这个顺序出现。
send 通过基类指针进入派生实现，打印成员 name_。
内层拥有者离开作用域时，虚析构先选择 FileChannel 的析构入口。
派生析构函数体结束后，本类成员自动销毁，然后再销毁 Channel 基类部分。
最后一行出现时，拥有者已经消失，外层计数器仍然存活。

两条析构计数都应为 1，程序用 assert 核对。
日志只打印函数体，不代表字符串成员没有析构。
本程序不手工调用基类析构，语言会自动完成正常链条。

## 按对象关系读代码

| 对象/成员 | 是否拥有相关资源 | 生命周期用途 |
|---|---|---|
| counts | 自己是外层计数值 | 必须活到两个析构完成之后 |
| channel | 独占拥有完整 FileChannel | 内层右花括号触发销毁 |
| name_ | 拥有名称字符串 | 派生成员自动析构 |
| counts_ | 借用计数器 | 引用销毁不销毁被引用对象 |

virtual 解决经基类删除时选择真实析构入口。
unique_ptr 的默认删除器不会替错误的基类契约兜底。
若改为非虚基类析构，经普通 Base* 删除真实 Derived 是未定义行为，
不能将其输出总结为“标准保证只执行 Base dtor”。

## 练习

1. 析构日志为何是派生先、基类后？
2. 如果把基类析构改为非虚并删掉派生析构 override，代码能编译就安全吗？
3. 为什么 counts 要放在内层作用域外？

> [!tip]- 展开练习答案
> 1. 虚析构先选真实派生入口；派生函数体和成员完成后，自动销毁基类。
> 2. 不安全。最终默认删除器经 Channel* 删除真实 FileChannel，违反 C++17 删除规则，是 UB。
> 3. 两个析构仍要写计数；计数器必须在它们执行时仍存活。
>
> 核对来源：[N4659 expr.delete/3](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/expr.delete#3)、[class.dtor](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.dtor)、[unique.ptr](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/unique.ptr)。

## 来源与边界

本页为独立撰写的学习示例，不冒称公司面试原题。
基线 C++17，固定使用 N4659 文本，核验日期为 **2026-10-21**。
关键语言规则：[N4659 expr.delete/3](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/expr.delete#3)、[class.dtor](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.dtor)、[unique.ptr](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/unique.ptr)。
工程建议与语言标准分开阅读，详见对应专题正文。

- [ ] 不看代码能说清每个对象的类型与寿命。
- [ ] 能按预期输出解释调用或销毁路径。
- [ ] 能区分编译错误、逻辑错误和未定义行为。
