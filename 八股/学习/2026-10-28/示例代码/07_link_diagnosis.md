# 练习：诊断一次 undefined reference · 完整源码阅读版

用途：面试官常给一段小代码问“为什么报错”。真正有价值的是先确定错误阶段，再说明证据与最小修复，而不是碰到 undefined reference 就机械添加头文件。

前置：[[学习/2026-10-28/07-练习-链接错误诊断|专题详解]]。源码与专题内代码块逐字一致；直接编译默认模式即可运行。

```cpp
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
```

## 读完检查

如果 `nm -C good.o` 有 `T rate(int)` 而 `bad.o` 只有 `U rate(int)`，这两个字母在 GNU nm 输出里意味着什么？

关键路径：先判断编译还是链接，再看调用处声明、实现处定义和链接输入；修复要让正确签名的定义进入最终链接，而非重复写声明。
