//
// Created by 20675 on 2026/10/16.
//
// 【痛点所在】：这里手动抄写了 io.cpp 中函数的签名（前向声明）
// 注意：这里不需要 #include <iostream>，因为 main 函数本身没用 cin/cout
int readNumber();
void writeAnswer(int x);

int main()
{
    int x { readNumber() };
    int y { readNumber() };
    writeAnswer(x + y);
    return 0;
}