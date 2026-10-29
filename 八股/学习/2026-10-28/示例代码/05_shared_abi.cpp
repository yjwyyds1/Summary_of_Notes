// 目的：展示一个 C 风格函数入口与共享库构建用的条件编译。
// 前置：知道 C++ 编译、链接和 Linux .so 是三个不同层面。
// 步骤：默认独立运行；定义 SHARED_LIBRARY_ONLY 可单独构建 .so。
#ifndef SHARED_LIBRARY_ONLY
#include <iostream>  // 独立演示模式打印结果。
#endif

// extern "C" 改变此函数的语言链接规则，不会替你搜索 .so 文件。
// int 参数只是教学接口；生产 C ABI 还要规定固定宽度、错误处理等。
extern "C" int quote_total(int price, int quantity) {
    return price * quantity;  // 教学输入小，暂不触及有符号溢出。
}

#ifndef SHARED_LIBRARY_ONLY
int main() {
    std::cout << quote_total(3, 4) << '\n';  // 预期输出 12。
}
#endif
