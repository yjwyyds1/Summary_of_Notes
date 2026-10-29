// 目的：用一个可运行文件分辨声明、普通定义与 C++17 inline 变量。
// 前置：理解每个 .cpp 独立编译；跨文件重复定义实验见专题正文。
// 步骤：声明 processed，提供唯一定义，修改后观察输出。
#include <iostream>  // std::cout 用于打印三个实体的值。

extern int processed;      // 仅声明：本句不创建另一个 processed 对象。
int processed = 0;         // 定义：本程序在这个翻译单元提供存储。
inline int per_program = 7; // C++17 内联变量可按规则放入多个翻译单元。
static int local_only = 1; // 内部链接；若放头文件，各翻译单元会各有一份。

int main() {
    ++processed;  // 修改前面定义的同一个对象。
    std::cout << processed << ',' << per_program << ',' << local_only << '\n';
}
