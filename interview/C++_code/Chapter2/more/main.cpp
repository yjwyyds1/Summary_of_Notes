//
// Created by 20675 on 2026/10/16.
//
#include "io.h"      // 【关键优化】：直接引入接口文档，不再手动写声明

int main()
{
    int x { readNumber() };
    int y { readNumber() };
    writeAnswer(x + y);
    return 0;
}