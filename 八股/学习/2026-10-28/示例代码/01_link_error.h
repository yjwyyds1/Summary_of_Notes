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
