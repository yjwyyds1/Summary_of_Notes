# 动态库搜索、PIC 与 extern C 边界 · 完整源码阅读版

用途：“能编译、能链接、却启动失败”常让初学者困惑。关键是分清三个时间点：构建时能找到库，程序启动时能找到库文件，调用时能找到匹配的导出符号。

前置：[[学习/2026-10-28/05-动态库与ABI|专题详解]]。源码与专题内代码块逐字一致；直接编译默认模式即可运行。

```cpp
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
```

## 读完检查

链接命令里有 `-L/tmp/lib`，部署时删除该目录后 app 为什么可能启动失败？`extern "C"` 能解决缺少共享对象文件的问题吗？

关键路径：先分链接时搜索和运行时装载；再按 `readelf -d` 检查依赖与路径；最后说明 PIC 是代码生成方式，extern C 是语言链接/符号边界，两个都不是库文件搜索开关。
