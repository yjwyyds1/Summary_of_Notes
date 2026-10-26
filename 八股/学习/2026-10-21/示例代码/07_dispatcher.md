---
date: 2026-10-21
id: CPP-POLY-PRACTICE-20261021
standard: C++17
status: 未自测
tags: [C++, 示例代码, 多态]
---

# 07_dispatcher · 当天综合练习正确解

入口：[[学习/学习导航/2026-10-21-学习导航|当天学习入口]]  
对应正文：[[学习/2026-10-21/06-对象切片与多态容器|专题详解]]  
源码：[[学习/2026-10-21/示例代码/07_dispatcher.cpp|07_dispatcher.cpp]]  
自测：[[八股测试/2026-10-21|当天测试页]]

## 用途与阅读前提

按题目固定契约，用一个抽象 PriceHandler 接口拥有 AddOne 与 DoublePrice。每个对象独立处理原始输入 100，结果是 101 和 200，并用 assert 核对。

程序是独立 C++17 翻译单元，单独编译，不与其他带 main 的示例拼接。
本页完整代码与同名 cpp、专题正文中的对应完整代码保持一致。
这里只列规则推导的预期输出；实际编译运行记录见当天统一验证结果。
不运行错误删除或其他未定义行为来猜固定输出。

## 完整中文注释源码

```cpp
// C++17 综合练习正确解：同一容器拥有不同处理器，各自处理原始价格 100。
// 输出 101、200；两个规则彼此独立，不是把 101 再交给 DoublePrice。

#include <cassert>   // 提供 assert：核对数量和业务结果。
#include <iostream>  // 提供 std::cout：逐行打印整数结果。
#include <memory>    // 提供 unique_ptr 和 make_unique：明确独占所有权。
#include <vector>    // 提供 vector：保存处理器拥有者和检查结果。

// PriceHandler 定义完整契约：允许多态销毁，并要求提供 const 处理函数。
class PriceHandler {
public:
    // public 允许默认删除器删除；virtual 确保真实派生对象完整销毁。
    // = default 让编译器生成适用的析构实现，不表示“非虚”。
    virtual ~PriceHandler() = default;

    // = 0 表示纯虚函数；PriceHandler 不能作为独立对象被实例化。
    // int price 是输入整数副本；const 表示不修改处理器的普通成员。
    virtual int handle(int price) const = 0;
};

// public 继承承诺 AddOne 可用作 PriceHandler；final 阻止继续派生。
class AddOne final : public PriceHandler {
public:
    // override 要求编译器检查签名；不能漏掉末尾 const。
    int handle(int price) const override {
        return price + 1;  // 契约要求：输入 100 时得到 101。
    }
};

// 第二种具体实现与第一种共享接口，但各自完成自己的计算。
class DoublePrice final : public PriceHandler {
public:
    int handle(int price) const override {
        return price * 2;  // 契约要求：输入 100 时得到 200。
    }
};

int main() {  // 程序入口；全部对象都由局部拥有者自动管理。
    // 外层 vector 管理指针元素；每个 unique_ptr 管理一个真实处理器。
    std::vector<std::unique_ptr<PriceHandler>> handlers;

    // 创建具体类型后把所有权转入基类智能指针，没有复制基类对象。
    handlers.push_back(std::make_unique<AddOne>());
    handlers.push_back(std::make_unique<DoublePrice>());

    std::vector<int> results;  // 保存结果，便于在打印之外进行断言。
    const int input = 100;     // 每个处理器都获得同一个原始价格。

    // const auto& 表示只读借用每个 unique_ptr，避免复制独占拥有者。
    // 注意：const unique_ptr 本身不等于“它指向的对象必为 const”。
    for (const auto& handler : handlers) {
        // -> 调用基类虚函数；真实对象分别选中 AddOne、DoublePrice。
        const int result = handler->handle(input);
        results.push_back(result);  // 保存整数值，不涉及多态对象复制。
        std::cout << result << '\n';
    }

    // 先核对元素数量，再用下标访问；避免验证代码自己越界。
    assert(results.size() == 2);
    assert(results[0] == 101);
    assert(results[1] == 200);
    return 0;  // 容器释放拥有者；虚析构保证每个真实处理器正确销毁。
}
```

## 预期输出

```text
101
200
```

## 输出理解

第一个拥有者指向 AddOne，handle(100) 返回 101。
第二个拥有者指向 DoublePrice，handle(100) 返回 200。
循环总是传 input，两个处理器不是依次传递结果的流水线。
若把第一个结果继续输入第二个处理器得到 202，就改变了题目契约。

PriceHandler 的 handle 是纯虚函数，因此它不能作为独立值保存。
vector<PriceHandler> 无法成为本题用于存放基类对象的正确容器。
错误版本若使用具体基类默认返回原价，按值切片才可能合法输出 100/100。
正确接口的编译错误与错误具体基类的合法切片，要分别分析。

## 六个知识点如何一起工作

| 本日知识 | 程序中的位置 |
|---|---|
| 公开继承 | AddOne/DoublePrice 是 PriceHandler |
| 虚函数与动态绑定 | 基类指针 handle 调用真实实现 |
| override | 检查参数、返回值与 const 的覆盖关系 |
| 抽象接口 | = 0 阻止把默认基类行为当成完整处理器 |
| 虚析构 | 基类拥有者能够正确销毁派生对象 |
| 避免切片 | 容器存 unique_ptr，真实处理器仍为派生对象 |

const auto& 只读借用每个 unique_ptr，避免复制独占拥有者。
这不表示所指对象自动变 const；handle 自己的 const 另有意义。
assert 先确认结果数量为 2，再访问下标 0、1。
定义 NDEBUG 的构建会关闭 assert；学习验证时保留断言。

演示只处理 100，结果在 int 范围内。
任意价格的单位、范围及溢出策略，应由后续实际业务定义。

## 练习

1. 新增 SubtractTwo，返回 price - 2，需要修改遍历吗？
2. 如果只把错误版本的 handle 加上 virtual，保留 vector<PriceHandler> 存具体基类值，够吗？
3. 如果只换成指针集合却仍保持非虚 handle，够吗？

> [!tip]- 展开练习答案
> 1. 不需要；增加实现并装入拥有者即可，输出加上 98，数量断言改为 3。
> 2. 不够。具体基类值仍会切片；若已经改为抽象基类，则不能创建这些值。
> 3. 不够。真实对象保留了，但非虚业务函数仍按静态类型选择实现。
>
> 核对来源：[N4659 class.abstract](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.abstract)、[class.virtual](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.virtual)、[expr.delete/3](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/expr.delete#3)。

## 来源与边界

本页为独立撰写的学习示例，不冒称公司面试原题。
基线 C++17，固定使用 N4659 文本，核验日期为 **2026-10-21**。
关键语言规则：[N4659 class.abstract](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.abstract)、[class.virtual](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.virtual)、[expr.delete/3](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/expr.delete#3)。
工程建议与语言标准分开阅读，详见对应专题正文。

- [ ] 不看代码能说清每个对象的类型与寿命。
- [ ] 能按预期输出解释调用或销毁路径。
- [ ] 能区分编译错误、逻辑错误和未定义行为。
