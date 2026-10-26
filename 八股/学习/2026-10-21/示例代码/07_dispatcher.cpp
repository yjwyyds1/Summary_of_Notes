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
