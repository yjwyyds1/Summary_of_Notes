# 静态库、归档抽取与链接顺序 · 完整源码阅读版

用途：构建大型项目时常把模块打包成库。`.a` 并不是把所有代码无条件复制进程序；常见 GNU 链接器按尚未解决的引用挑成员。理解这个差异，能解释看似“明明加了库还报 undefined reference”。

前置：[[学习/2026-10-28/04-静态库与链接顺序|专题详解]]。源码与专题内代码块逐字一致；直接编译默认模式即可运行。

```cpp
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
```

## 读完检查

若 `liba.a` 里的成员引用 `libb.a`，通常哪个放后面？`-L./build` 能让运行中的动态链接器找到 `./build/libx.so` 吗？

关键路径：先说 `.a` 是目标文件归档；再说 GNU ld 扫描归档时只抽取当下所需成员；最后用“引用者在前、提供库在后”解释顺序问题。
