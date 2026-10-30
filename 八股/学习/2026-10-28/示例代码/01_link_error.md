---
date: 2026-10-28
type: 代码阅读版
source: 01_link_error_main.cpp / 01_link_error_util.cpp / 01_link_error.h
---

# 01_link_error：只有声明会怎样

← [[学习/2026-10-28/03-声明定义与ODR|对应专题]] · [[学习/2026-10-28/07-练习-链接错误诊断|练习]] · [[学习/学习导航/2026-10-28-学习导航|今日导航]]

## 这份代码在做什么

三个文件构成最小工程：

- `01_link_error.h`：只有声明，带 include guard；
- `01_link_error_main.cpp`：引用 `add` / `add_scaled`；
- `01_link_error_util.cpp`：提供定义。

先只编译 main 并链接，复现 `undefined reference`；再把两个 `.cpp` 一起链接，程序正常输出。

## 复现与修复

```bash
# 1. 只编译引用方，必然失败
g++ -std=c++17 -O0 01_link_error_main.cpp -o link_demo

# 2. 看符号状态：add 是 U（未定义）
g++ -std=c++17 -O0 -c 01_link_error_main.cpp -o main_only.o
nm -C main_only.o | grep add

# 3. 补上实现，成功
g++ -std=c++17 -O0 -Wall -Wextra 01_link_error_main.cpp 01_link_error_util.cpp -o link_demo
./link_demo
```

## 本机实际输出

失败时（GCC 13.1 / MinGW）：

```text
ld.exe: ...01_link_error_main.cpp:(.text+0x18): undefined reference to `add(int, int)'
ld.exe: ...01_link_error_main.cpp:(.text+0x2a): undefined reference to `add_scaled(int, int)'
collect2.exe: error: ld returned 1 exit status
```

符号状态：

```text
                 U add_scaled(int, int)
                 U add(int, int)
```

修复后：

```text
add(1, 2) = 3
add_scaled(3) = 6
```

## 完整源码（与各 `.cpp` / `.h` 逐字一致）

### 01_link_error.h

```cpp
// ============================================================================
// 01_link_error.h
// 目的：演示“只有声明、没有定义”会怎样——编译能过，链接失败。
// 用法：被 01_link_error_main.cpp 包含；定义在 01_link_error_util.cpp。
// 注意：这里故意只放声明。把函数体写进头文件，反而会造成 multiple definition。
// ============================================================================

#ifndef LINK_DEMO_MATH_UTIL_H   // include guard：防止同一个翻译单元重复包含
#define LINK_DEMO_MATH_UTIL_H

// 求和：声明只告诉编译器“有这个名字、参数是 int、返回 int”
int add(int lhs, int rhs);

// 带默认参数的版本：默认值只写在声明处
int add_scaled(int value, int scale = 2);

#endif  // LINK_DEMO_MATH_UTIL_H
```

### 01_link_error_main.cpp

```cpp
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
```

### 01_link_error_util.cpp

```cpp
// ============================================================================
// 01_link_error_util.cpp
// 目的：提供 01_link_error.h 里声明的函数定义。
//       没有它，链接器找不到 add / add_scaled，会报 undefined reference。
// 说明：默认参数只写在头文件的声明里，定义处不再重复写默认值。
// ============================================================================

#include "01_link_error.h"

// 定义：函数体在这里，链接器才能把 main.o 里的 U（未定义）符号补上
int add(int lhs, int rhs) {
    return lhs + rhs;
}

// 注意：这里不能再写 “int scale = 2”，默认参数只能出现一次
int add_scaled(int value, int scale) {
    return value * scale;
}
```

## 怎么读结果

- 编译阶段没有任何错误：声明足够让编译器生成调用指令。
- 链接阶段报 `undefined reference`：链接器在符号表里找不到 `add` 的定义。`nm` 的 `U` 就是这个状态。
- 修复只需要把定义所在的翻译单元加入链接命令；顺序问题、库缺失、名字修饰不符会产生同类报错，处理方式见 [[学习/2026-10-28/03-声明定义与ODR|ODR 专题]]。

## 对应测试题

[[八股测试/2026-10-28#EX-20261028-001 · 链接错误诊断]]
