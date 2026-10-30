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
