// 目的：复现“声明存在、定义缺失”导致的链接错误，默认模式安全可运行。
// 前置：知道 -D 宏影响条件编译；故障模式只用于观察链接诊断。
// 步骤：默认构建运行；再添加 -DDEMO_MISSING_DEFINITION 构建并查看错误。
#include <iostream>  // std::cout 用于默认模式输出。

int rate(int quantity);  // 声明让 main 知道参数与返回类型，但它不是实现。

#ifndef DEMO_MISSING_DEFINITION
int rate(int quantity) {  // 默认模式提供唯一定义。
    return quantity * 2;  // 输入 5 时返回 10。
}
#endif

int main() {
    std::cout << rate(5) << '\n';  // 故障宏开启后，这个引用没有定义可链接。
}
