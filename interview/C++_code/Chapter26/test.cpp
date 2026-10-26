//
// Created by 20675 on 2026/10/25.
//
#include <iostream>

int main()
{
    // 打印当前实际使用的 C++ 标准
#if __cplusplus >= 202002L
    std::cout << "当前是 C++20 或更高\n";
#elif __cplusplus >= 201703L
    std::cout << "当前是 C++17\n";
#elif __cplusplus >= 201402L
    std::cout << "当前是 C++14\n";
#elif __cplusplus >= 201103L
    std::cout << "当前是 C++11\n";
#else
    std::cout << "当前是 C++98 或更早\n";
#endif

    std::cout << "__cplusplus 宏的值 = " << __cplusplus << std::endl;

    // 后面你原来的代码...
}