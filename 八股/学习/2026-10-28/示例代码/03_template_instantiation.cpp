// 目的：展示模板定义、extern template 声明和显式实例化定义。
// 前置：理解函数声明与定义；本文件可独立运行。
// 步骤：推导 T=int 并调用已生成的特化，观察结果。
#include <iostream>  // std::cout 打印结果。

// T 是类型参数；编译器按使用的具体 T 处理模板特化。
template <class T>
T twice(T value) { return value + value; }

// 此句是显式实例化声明，不生成 int 版本的函数体。
extern template int twice<int>(int);
// 此句是显式实例化定义，要求在此生成 int 版本。
template int twice<int>(int);

int main() {
    const int input = 21;  // const 限定这个局部变量不可改。
    std::cout << twice(input) << '\n';  // T 被推导为 int，预期输出 42。
}
