//
// Created by 20675 on 2026/10/16.
//
#include <iostream>

int readNumber()
{
    std::cout << "Please enter a number: ";
    int x {};
    std::cin >> x;
    return x;
}

void writeAnswer(int x)
{
    std::cout << "The quotient is: " << x << '\n';
}

int main()
{
    int x{ };
    int y{ };
    x = readNumber();
    x = readNumber();
    writeAnswer(x/y);

    return 0;
}

/*

SIGFPE (Arithmetic exception)

全称是 Signal Floating Point Exception（浮点异常信号），但在实际开发中，它绝大多数时候是由整数除以零触发的

D:\Algorithm\Chapter3\Question_2.exe
Please enter a number:1

Please enter a number:3


进程已结束，退出代码为 -1073741676 (0xC0000094)
*/