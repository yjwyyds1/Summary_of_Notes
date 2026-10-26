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
