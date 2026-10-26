---
date: 2026-10-21
id: CPP-SLICE-001
standard: C++17
status: 未自测
tags: [C++, 示例代码, 多态]
---

# 06_slicing · 值、借用与拥有型多态容器

入口：[[学习/学习导航/2026-10-21-学习导航|当天学习入口]]  
对应正文：[[学习/2026-10-21/06-对象切片与多态容器|专题详解]]  
源码：[[学习/2026-10-21/示例代码/06_slicing.cpp|06_slicing.cpp]]  
自测：[[八股测试/2026-10-21|当天测试页]]

## 用途与阅读前提

对比复制基类值、按值形参、按引用形参、vector<PriceRule> 和 vector<unique_ptr<PriceRule>>。为了能演示合法切片，本文件基类有默认实现且显式允许复制。

程序是独立 C++17 翻译单元，单独编译，不与其他带 main 的示例拼接。
本页完整代码与同名 cpp、专题正文中的对应完整代码保持一致。
这里只列规则推导的预期输出；实际编译运行记录见当天统一验证结果。
不运行错误删除或其他未定义行为来猜固定输出。

## 完整中文注释源码

```cpp
// C++17：比较基类值、基类引用和拥有基类指针时的对象切片。
// 本文件故意允许基类复制来展示合法但常常不符合业务意图的切片。

#include <cassert>   // 提供 assert：分别核对四条路径的结果。
#include <iostream>  // 提供 std::cout：输出结果供手动对照。
#include <memory>    // 提供 unique_ptr、make_unique：保存真实派生对象。
#include <vector>    // 提供 vector：分别存放基类值和拥有型指针。

// 这里的 PriceRule 是具体类，便于演示“确实能复制出一个基类对象”。
// 它与 04 的同名类位于不同独立程序，不要把两个 cpp 拼成一个文件。
class PriceRule {
public:
    PriceRule() = default;  // 明确提供无参构造，创建普通基类对象。
    // 教学目的显式允许复制；真实多态接口常会禁止公开复制来防切片。
    PriceRule(const PriceRule&) = default;
    virtual ~PriceRule() = default;  // 支持 unique_ptr<PriceRule> 的多态销毁。

    // 有函数体，因此本类不是抽象类；默认策略就是原价返回。
    virtual int quote(int price) const {
        return price;
    }
};

// FeeRule 的额外状态 fee_ 不属于一个独立 PriceRule 对象。
class FeeRule final : public PriceRule {
public:
    // explicit 避免整数自动转成规则；初始化列表直接设置费用成员。
    explicit FeeRule(int fee) : fee_(fee) {}

    int quote(int price) const override {
        return price + fee_;  // 只有真实 FeeRule 对象才使用此覆盖实现。
    }

private:
    int fee_;  // 派生类新增状态，按值复制成基类时不会被保存。
};

// 形参写成 PriceRule：调用前先创建一个独立的基类形参对象。
int by_value(PriceRule rule, int price) {
    return rule.quote(price);  // rule 的真实类型已经是 PriceRule。
}

// const PriceRule&：借用原对象，没有创建新的基类对象，也不取得所有权。
int by_reference(const PriceRule& rule, int price) {
    return rule.quote(price);  // 原对象若为 FeeRule，就进入其覆盖实现。
}

int main() {  // 每个独立 cpp 都各自有一个 main。
    FeeRule original{3};  // 真实类型为 FeeRule，费用状态为 3。

    // 目标变量声明为 PriceRule，只复制 original 中的基类部分。
    PriceRule sliced = original;
    const int value_result = sliced.quote(100);
    const int parameter_result = by_value(original, 100);
    const int reference_result = by_reference(original, 100);

    // vector 的元素是 PriceRule 本体；push_back 同样会创建基类值。
    std::vector<PriceRule> values;
    values.push_back(original);
    const int vector_value_result = values.front().quote(100);

    // 元素换成拥有型指针：容器保存指针，对象本身仍是完整 FeeRule。
    std::vector<std::unique_ptr<PriceRule>> owners;
    owners.push_back(std::make_unique<FeeRule>(3));

    // front() 取第一个 unique_ptr；-> 通过基类指针虚调用真实实现。
    const int vector_owner_result = owners.front()->quote(100);

    // 切片路径不会自动报错；断言用于明确这里的可预期语言行为。
    assert(value_result == 100);
    assert(parameter_result == 100);
    assert(reference_result == 103);
    assert(vector_value_result == 100);
    assert(vector_owner_result == 103);

    std::cout << "value: " << value_result << '\n';
    std::cout << "parameter: " << parameter_result << '\n';
    std::cout << "reference: " << reference_result << '\n';
    std::cout << "vector-value: " << vector_value_result << '\n';
    std::cout << "vector-owner: " << vector_owner_result << '\n';
    return 0;  // owners 自动销毁真实 FeeRule，随后销毁其基类部分。
}
```

## 预期输出

```text
value: 100
parameter: 100
reference: 103
vector-value: 100
vector-owner: 103
```

## 输出理解

value 和 parameter 都创建独立的 PriceRule 值，得到基类默认行为。
reference 借用原来的 FeeRule，派生费用状态仍在，结果为 103。
vector-value 的元素声明为基类本体，插入时同样创建基类对象。
vector-owner 的元素是智能指针，真实对象仍然是 FeeRule。

切片不会破坏 original；它仍是费用为 3 的 FeeRule。
新基类对象没有 fee_，其真实类型也不是 FeeRule。
这里 virtual 一直存在，但不会让基类值“恢复”派生身份。
这些结果来自合法对象复制，不是运行一次 UB 得出的经验结论。

## 逐处寻找复制目标

| 代码 | 新建的对象/访问关系 | 是否保留原动态类型 |
|---|---|---|
| PriceRule sliced = original | 新建 PriceRule 值 | 否 |
| by_value(original, 100) | 新建 PriceRule 形参 | 否 |
| by_reference(original, 100) | 引用原 FeeRule | 是 |
| values.push_back(original) | 新建 PriceRule 元素 | 否 |
| owners.push_back(make_unique<FeeRule>(3)) | 保存拥有完整 FeeRule 的指针 | 是 |

把 quote 改为纯虚之后，不能再创建这些独立基类值。
那时应分析编译错误，不能继续预测它输出 100。
真实多态接口常通过禁止公开复制来减少这种误用，
本例公开复制只为把语言机制显示出来。

## 练习

1. 如果去掉 PriceRule::quote 的 virtual，by_reference 还会得到 103 吗？
2. 把 sliced 再绑定到 const PriceRule&，能恢复 FeeRule 吗？
3. vector 改存拥有者后，基类析构为什么还要 virtual？

> [!tip]- 展开练习答案
> 1. 不会，普通非虚成员根据静态接口选择，引用路径也调用基类实现。
> 2. 不能；它只是引用一个已经存在的真实 PriceRule 对象。
> 3. unique_ptr<PriceRule> 的默认删除器经基类指针删除真实派生对象，需要正确虚析构。
>
> 核对来源：[N4659 class.copy.ctor/14](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.copy.ctor#14)、[class.abstract](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.abstract)、[expr.delete/3](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/expr.delete#3)。

## 来源与边界

本页为独立撰写的学习示例，不冒称公司面试原题。
基线 C++17，固定使用 N4659 文本，核验日期为 **2026-10-21**。
关键语言规则：[N4659 class.copy.ctor/14](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.copy.ctor#14)、[class.abstract](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/class.abstract)、[expr.delete/3](https://www.ece.uvic.ca/~frodo/cppdraft/n4659/html/expr.delete#3)。
工程建议与语言标准分开阅读，详见对应专题正文。

- [ ] 不看代码能说清每个对象的类型与寿命。
- [ ] 能按预期输出解释调用或销毁路径。
- [ ] 能区分编译错误、逻辑错误和未定义行为。
