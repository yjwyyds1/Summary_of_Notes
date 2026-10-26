//
// Created by 20675 on 2026/10/16.
//
#include <iostream>
void a();
void d()
{ // here
}

void c()
{
    a();
}

void b()
{
    c();
    d();
}

void a()
{
    b();
}

int main()
{
    a();

    return 0;
}

/*
SIGSEGV (Segmentation fault)
这个 SIGSEGV 是由无限递归 (Infinite Recursion) 导致的栈溢出 (Stack Overflow)
 */