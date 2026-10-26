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
