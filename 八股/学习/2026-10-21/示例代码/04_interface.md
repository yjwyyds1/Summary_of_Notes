---
date: 2026-10-21
id: CPP-ABSTRACT-001
standard: C++17
status: 未自测
tags: [C++, 示例代码, 多态]
---

# 04_interface · 抽象接口与引用注入

入口：[[学习/学习导航/2026-10-21-学习导航|当天学习入口]]  
对应正文：[[学习/2026-10-21/04-抽象类与接口|专题详解]]  
源码：[[学习/2026-10-21/示例代码/04_interface.cpp|04_interface.cpp]]  
自测：[[八股测试/2026-10-21|当天测试页]]

## 用途与阅读前提

用同一个 QuoteService，分别借用 KeepPrice 和 AddFee 两个具体实现，观察统一接口如何选择实际行为。本程序不拥有规则，不创建抽象类本体。

程序是独立 C++17 翻译单元，单独编译，不与其他带 main 的示例拼接。
本页完整代码与同名 cpp、专题正文中的对应完整代码保持一致。
这里只列规则推导的预期输出；实际编译运行记录见当天统一验证结果。
不运行错误删除或其他未定义行为来猜固定输出。

## 完整中文注释源码

```cpp
// C++17：用抽象接口连接业务与可替换的价格规则。
// 本程序只演示正常执行；不包含未定义行为或故意编译失败的语句。

#include <cassert>   // 提供 assert：检查示例的价格结果。
#include <iostream>  // 提供 std::cout：把结果写到标准输出。

// PriceRule 表示“能根据输入价格给出报价”的统一约定。
// class 声明类；成员默认 private，因此在下面显式写 public。
class PriceRule {
public:
    // ~PriceRule 是析构函数；没有返回类型，也没有参数。
    // virtual 允许经 PriceRule* 销毁真实派生对象；= default 使用默认实现。
    virtual ~PriceRule() = default;

    // virtual 允许派生类提供实现；int 是返回的整数价格类型。
    // int price 是按值传入的原价格，函数内部拿到一份整数副本。
    // 末尾 const 表示本次调用不修改对象的普通成员。
    // = 0 是纯虚说明符，不是“让返回值等于零”，也不是变量赋值。
    virtual int quote(int price) const = 0;
};  // 类定义结束后必须有分号。

// public PriceRule 表示 KeepPrice 可以作为 PriceRule 使用。
// final 只表示本示例不允许继续继承 KeepPrice，不负责开启多态。
class KeepPrice final : public PriceRule {
public:
    // override 要求编译器确认签名确实覆盖了基类的虚函数。
    // 即使省略 virtual，正确的覆盖函数仍然是虚函数。
    int quote(int price) const override {
        return price;  // 不加任何费用，原样返回。
    }
};

// AddFee 也是一种 PriceRule；它需要额外保存“加多少”这个状态。
class AddFee final : public PriceRule {
public:
    // 构造函数没有返回类型；explicit 避免整数隐式变成 AddFee 对象。
    // 冒号后的 fee_(fee) 是成员初始化列表，初始化成员 fee_。
    explicit AddFee(int fee) : fee_(fee) {}

    // const 与基类声明一致，避免不小心变成另一个函数。
    int quote(int price) const override {
        return price + fee_;  // 用派生对象自身保存的费用计算结果。
    }

private:
    int fee_;  // 每个 AddFee 对象独立持有的加价金额。
};

// QuoteService 是使用规则的业务类，不是“某一种价格规则”。
// 它通过一个引用成员使用规则：这是组合关系中的非拥有借用。
class QuoteService {
public:
    // const PriceRule& 是对已有规则的只读引用，不复制抽象对象。
    // 调用者在构造服务时提供规则，这个动作叫“注入依赖”。
    explicit QuoteService(const PriceRule& rule) : rule_(rule) {}

    // int price 是业务输入；const 说明服务本身也不修改普通成员。
    int run(int price) const {
        // rule_ 的声明类型统一，真实对象决定 quote 的最终实现。
        return rule_.quote(price);
    }

private:
    // 引用不是所有者；被引用规则必须在本服务使用期间持续存活。
    const PriceRule& rule_;
};

// main 是程序入口；返回 int，0 表示正常结束。
int main() {
    // 先创建规则、再创建引用它们的服务，确保服务使用时规则仍存活。
    KeepPrice keep;
    AddFee fee{3};  // 花括号把整数 3 交给 explicit 构造函数。

    // 两个服务的代码一样，区别仅在注入的真实规则对象。
    QuoteService plain_service{keep};
    QuoteService fee_service{fee};

    const int plain = plain_service.run(100);  // 调用 KeepPrice::quote。
    const int charged = fee_service.run(100);  // 调用 AddFee::quote。

    // assert 用于学习示例的结果核对，不代替正式业务的输入校验。
    assert(plain == 100);
    assert(charged == 103);

    // '\n' 表示换行；输出文字是结果，不是类名的自动识别。
    std::cout << plain << '\n';
    std::cout << charged << '\n';
    return 0;  // 局部对象离开作用域时由语言规则自动销毁。
}
```

## 预期输出

```text
100
103
```

## 输出理解

第一行来自 KeepPrice：100 原样返回。
第二行来自 AddFee{3}：原价 100 加上费用 3，得到 103。
输出差异来自具体规则；QuoteService::run 本身没有按类型分支。

对象创建顺序是 keep、fee、plain_service、fee_service。
服务只借用规则，所以要保证调用期间两个规则都存活。
main 中的局部创建顺序和使用位置满足这个前提。

## 按语法读代码

| 位置 | 读法与目的 |
|---|---|
| public | 把接口操作提供给外部服务调用 |
| virtual int quote | 声明可由具体类覆盖的业务操作 |
| const = 0 | 只读操作且仍需具体实现；不是返回 0 |
| virtual ~PriceRule | 单独提供经基类销毁的契约 |
| explicit AddFee(int fee) | 只接受明确构造，初始化费用状态 |
| fee_(fee) | 初始化成员，不是在函数体里赋值 |
| const PriceRule& | 借用已存在的具体对象，不复制抽象对象 |
| rule_.quote(price) | 通过接口引用进行动态调用 |

这里的接口无数据，是本例的设计选择。
抽象类本身可以有状态、普通函数和构造函数。
抽象类不能创建独立对象，但指针与引用完全可以表示它的接口。

## 练习

1. 把 AddFee{3} 改成 AddFee{5}，两行结果各是多少？
2. 删除 AddFee::quote 末尾 const，并保留 override，会发生什么？
3. 服务引用的局部规则先销毁，还能继续调用 run 吗？

> [!tip]- 展开练习答案
> 1. 100、105。服务没有改变，规则自身的状态改变。
> 2. 不能正确覆盖 const 版本，override 让编译器报错。
> 3. 不能；服务只借用，引用会悬垂，接口不会延长原对象寿命。
>
> 核对来源：[N4659 class.abstract](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.abstract)、[class.virtual](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.virtual)。

## 来源与边界

本页为独立撰写的学习示例，不冒称公司面试原题。
基线 C++17，固定使用 N4659 文本，核验日期为 **2026-10-21**。
关键语言规则：[N4659 class.abstract](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.abstract)、[class.virtual](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.virtual)。
工程建议与语言标准分开阅读，详见对应专题正文。

- [ ] 不看代码能说清每个对象的类型与寿命。
- [ ] 能按预期输出解释调用或销毁路径。
- [ ] 能区分编译错误、逻辑错误和未定义行为。
