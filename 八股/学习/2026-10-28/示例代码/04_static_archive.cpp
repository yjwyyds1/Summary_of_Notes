// 目的：提供一个可打包进静态库的函数，同时保持默认独立可运行。
// 前置：知道 .o 是目标文件、.a 是目标文件归档。
// 步骤：默认直接运行；定义 ARCHIVE_LIBRARY_ONLY 时仅编译库函数。
#ifndef ARCHIVE_LIBRARY_ONLY
#include <iostream>  // 默认演示模式使用 std::cout；库模式不需要它。
#endif

// fee 的定义可被另一个目标文件引用；这里用小整数避免溢出。
int fee(int quantity) { return quantity * 2; }

#ifndef ARCHIVE_LIBRARY_ONLY
int main() {
    const int quantity = 3;  // 教学输入，预期 fee 返回 6。
    std::cout << fee(quantity) << '\n';
}
#endif
