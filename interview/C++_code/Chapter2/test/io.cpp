//
// Created by 20675 on 2026/10/16.
//
#include <iostream> // 因为这里用到了 cin 和 cout，所以必须包含

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