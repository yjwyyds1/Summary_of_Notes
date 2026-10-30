// ============================================================================
// 01_link_error_main.cpp
// 目的：作为“引用方”，验证链接错误与修复。
// 步骤：
//   1. 只编译本文件并链接：应报 undefined reference to add(int, int)；
//   2. 加上 01_link_error_util.cpp 一起编译链接：正常输出。
// 编译（会失败）：g++ -std=c++17 -O0 01_link_error_main.cpp -o link_demo
// 编译（会成功）：g++ -std=c++17 -O0 01_link_error_main.cpp 01_link_error_util.cpp -o link_demo
// ============================================================================

#include <iostream>   // std::cout

#include "01_link_error.h"   // 只有声明，没有定义

int main() {
    const int sum = add(1, 2);
    const int scaled = add_scaled(3);   // 使用默认参数 scale = 2
    std::cout << "add(1, 2) = " << sum << "\n";
    std::cout << "add_scaled(3) = " << scaled << "\n";
    return 0;
}
