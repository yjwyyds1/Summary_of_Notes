// 目的：观察宏、C++ 常量、函数声明/定义分别在哪个阶段发挥作用。
// 前置：会运行 g++；本文件独立可编译，推荐 C++17。
// 步骤：先预处理，再编译为 .o，最后链接运行；观察各产物。
#include <iostream>  // std::cout 输出最终计算结果。

#define MARKET_NAME "demo"  // 宏在预处理阶段做令牌替换，不具备 C++ 类型。

// 这是函数声明：告诉调用点参数与返回类型，但不提供函数体。
int quote_value(int price, int lot);

int main() {
    constexpr int lot = 100;  // C++ 常量有 int 类型，编译器会按语言规则检查。
    const int price = 3;      // const 表示不能通过这个名字修改 price。
    std::cout << MARKET_NAME << ':' << quote_value(price, lot) << '\n';
}

// 这是函数定义：链接时 quote_value 的引用最终需要找到这样的实现。
int quote_value(int price, int lot) {
    return price * lot;  // 这里只演示正常的小整数输入，不涉及溢出边界。
}
