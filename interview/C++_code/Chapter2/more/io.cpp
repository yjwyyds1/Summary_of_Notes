//
// Created by 20675 on 2026/10/16.
//
#include "io.h"      // 【关键优化】：包含自己的头文件
#include <iostream>  // 包含实现所需的系统库

// 函数的具体实现（定义）
int readNumber()
{
    std::cout << "Enter a number to add: ";
    int x {};
    std::cin >> x;
    return x;
}

void writeAnswer(int x)
{
    std::cout << "The answer is " << x << '\n';
}