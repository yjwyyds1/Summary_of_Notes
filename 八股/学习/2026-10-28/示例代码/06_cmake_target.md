# CMake target 与依赖传播 · 完整源码阅读版

用途：真实 C++ 项目经常不是手写一行 g++，而是 CMake 管理多个模块。看到 `target_link_libraries` 和 `target_include_directories`，要把它们当成依赖图的边与接口契约，而非神秘咒语。

前置：[[学习/2026-10-28/06-CMake目标与依赖|专题详解]]。源码与专题内代码块逐字一致；直接编译默认模式即可运行。

```cpp
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
```

## 读完检查

如果宏只在库内部 `.cpp` 使用，为什么应优先 PRIVATE？若公共头文件里的条件编译也依赖该宏，又该如何改？

关键路径：先把 target 视为构建图节点，再说明 PUBLIC/PRIVATE/INTERFACE 的属性范围，最后用公共头文件是否需要该依赖来决定是否传播。
