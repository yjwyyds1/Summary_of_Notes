// ============================================================================
// 01_vtable_layout.cpp
// 目的：用可运行的小程序观察 C++ 对象的“多态成本”与常见实现下的内存布局。
// 前置：读过虚函数、继承、sizeof 的基本概念（见当天 01 与 05 号专题）。
// 程序步骤：
//   1. 对比“没有虚函数”与“有虚函数”的类的大小；
//   2. 观察同一个类的两个对象是否共享同一张虚表；
//   3. 观察单继承、多继承、菱形虚继承下对象大小的变化；
//   4. 通过虚函数调用验证“按对象真实类型派发”。
// 观察点：
//   - 具体数值依赖编译器/ABI（本机为 GCC 13.1 + Itanium C++ ABI + x86-64）；
//   - 标准只规定“行为”，不规定虚表的内存形状，本文标注的布局属于常见实现。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 01_vtable_layout.cpp -o 01_vtable_layout
// ============================================================================

#include <cstdint>   // uintptr_t：能装下一个指针的整数类型，只用于打印地址
#include <cstring>   // memcpy：把对象头几个字节原样复制到指针变量，便于观察
#include <iostream>  // std::cout：输出观察结果

// ---------------------------------------------------------------------------
// 1) 普通类：没有虚函数，就没有“按运行时类型派发”的需求
// ---------------------------------------------------------------------------
class Plain {
public:
    void hello() const { std::cout << "Plain::hello\n"; }
private:
    int value_ = 0;  // 一个 int 成员，占 4 字节
};

// ---------------------------------------------------------------------------
// 2) 多态类：只要类里有虚函数，每个对象就需要知道“该用哪张表”
// ---------------------------------------------------------------------------
class Base {
public:
    virtual ~Base() = default;          // 虚析构：经基类指针 delete 时能销毁派生部分
    virtual void speak() const { std::cout << "Base::speak\n"; }
    virtual int id() const { return 1; }
private:
    int base_value_ = 0;                // 派生类对象里也会包含这一份成员
};

class Derived : public Base {
public:
    void speak() const override { std::cout << "Derived::speak\n"; }  // 覆盖基类版本
    int id() const override { return 2; }
private:
    int derived_value_ = 0;
};

// ---------------------------------------------------------------------------
// 3) 多继承：两个基类各自带虚函数时，派生对象要同时能“当作 A”和“当作 B”
// ---------------------------------------------------------------------------
class A {
public:
    virtual ~A() = default;
    virtual void fa() { std::cout << "A::fa\n"; }
private:
    int a_ = 0;
};

class B {
public:
    virtual ~B() = default;
    virtual void fb() { std::cout << "B::fb\n"; }
private:
    int b_ = 0;
};

class C : public A, public B {
public:
    void fa() override { std::cout << "C::fa\n"; }
    void fb() override { std::cout << "C::fb\n"; }
private:
    int c_ = 0;
};

// ---------------------------------------------------------------------------
// 4) 菱形虚继承：L 和 R 都虚继承 VBase，D 里只保留一份 VBase
//    常见实现会用“虚基类偏移”记录 VBase 子对象的位置，因此对象更大。
// ---------------------------------------------------------------------------
class VBase {
public:
    virtual ~VBase() = default;
    virtual void hv() { std::cout << "VBase::hv\n"; }
private:
    int v_ = 0;
};

class L : virtual public VBase {
private:
    int l_ = 0;
};

class R : virtual public VBase {
private:
    int r_ = 0;
};

class Diamond : public L, public R {
private:
    int d_ = 0;
};

// 读出“对象头部的第一个机器字”，在 Itanium ABI 下它就是 vptr。
// 注意：这是实现相关的观察手段，不是标准保证；只用来对比，不用于业务代码。
static std::uintptr_t head_word(const void* object) {
    std::uintptr_t word = 0;
    std::memcpy(&word, object, sizeof(word));  // 只复制前 8 字节（x86-64）
    return word;
}

int main() {
    std::cout << "=== 1. 大小对比（单位：字节，x86-64 / GCC 13.1）===\n";
    std::cout << "sizeof(Plain)      = " << sizeof(Plain) << "   // 只有 4 字节成员，对齐到 4\n";
    std::cout << "sizeof(Base)       = " << sizeof(Base) << "   // 成员 4 字节 + 虚表指针 8 字节，再补齐\n";
    std::cout << "sizeof(Derived)    = " << sizeof(Derived) << "  // 基类子对象 + 自己的 4 字节成员\n";
    std::cout << "sizeof(A)          = " << sizeof(A) << "  // 单继承链上的单个虚表指针\n";
    std::cout << "sizeof(C)          = " << sizeof(C) << "  // 两个多态基类 ⇒ 常见实现有两个虚表指针\n";
    std::cout << "sizeof(Diamond)    = " << sizeof(Diamond) << "  // 虚继承额外保存虚基类偏移\n";

    std::cout << "\n=== 2. 同一类的多个对象共享同一张虚表 ===\n";
    Base b1;
    Base b2;
    Derived d1;
    std::cout << std::hex << std::showbase;
    std::cout << "b1 头字 = " << head_word(&b1) << "\n";
    std::cout << "b2 头字 = " << head_word(&b2) << "   // 与 b1 相同：虚表属于类，不属于对象\n";
    std::cout << "d1 头字 = " << head_word(&d1) << "   // 与 b1 不同：Derived 有自己的虚表\n";
    std::cout << std::dec << std::noshowbase;

    std::cout << "\n=== 3. 调用时按对象的真实类型派发 ===\n";
    Base* pointer = &d1;      // 静态类型是 Base*，指向的却是 Derived 对象
    pointer->speak();         // 输出 Derived::speak：通过虚表找到 Derived 的版本
    const Base& reference = b1;
    reference.speak();        // 输出 Base::speak

    std::cout << "\n=== 4. 多继承下两个基类指针指向同一对象的不同子对象 ===\n";
    C object;
    A* as_a = &object;
    B* as_b = &object;
    std::cout << "&object      = " << static_cast<const void*>(&object) << "\n";
    std::cout << "A* 指向      = " << static_cast<const void*>(as_a) << "\n";
    std::cout << "B* 指向      = " << static_cast<const void*>(as_b) << "  // 常见实现会偏移到 B 子对象\n";
    as_a->fa();
    as_b->fb();

    std::cout << "\n=== 5. 虚继承下最派生对象只有一份 VBase ===\n";
    Diamond diamond;
    VBase* vb = &diamond;
    vb->hv();
    std::cout << "sizeof 增加的字节来自虚基类偏移与对齐，而不是又多了一份 v_ 成员\n";
    return 0;
}
