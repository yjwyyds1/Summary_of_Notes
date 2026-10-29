// 目的：观察构建系统传入的编译宏如何改变一个翻译单元。
// 前置：知道预处理宏和 target_compile_definitions 的关系。
// 步骤：默认编译输出 6；加入 -DBOOK_SCALE=2 后输出 12。
#include <iostream>  // std::cout 打印最终结果。

#ifndef BOOK_SCALE
#define BOOK_SCALE 1  // 没有构建系统参数时给独立运行提供默认值。
#endif

#if BOOK_SCALE < 1
#error BOOK_SCALE_must_be_positive  // 在预处理阶段直接拒绝无效配置。
#endif

int scaled_fee(int price, int quantity) {
    return price * quantity * BOOK_SCALE;  // 宏在编译前展开；教学输入很小。
}

int main() {
    std::cout << scaled_fee(2, 3) << '\n';  // 默认 6；传 -DBOOK_SCALE=2 则为 12。
}
