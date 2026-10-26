---
date: 2026-10-21
standard: C++17
status: 未自测
---

# 02_virtual_dispatch · 完整中文注释源码

[[学习/学习导航/2026-10-21-学习导航|当天学习入口]] · [[学习/2026-10-21/02-虚函数与动态绑定|对应学习正文]]

[[八股测试/2026-10-21#CPP-VIRTUAL-001 · 同一个对象，经不同类型调用时选哪个函数|对应自测]]

以下围栏中的内容与同名 .cpp 完整逐字一致，可复制为单独的 C++17 程序。

```cpp
// C++17：用同一个对象观察静态选择、虚分派与构造析构阶段。
#include <cassert>  // assert 检查本例写出的语义预期。
#include <iostream> // std::cout 输出与断言对应的简短结果。

class Base {
public:
    // Base 构造期间，调用本对象虚函数会选 Base 层的实现。
    Base() { assert(value() == 10); }

    // virtual 使匹配的派生类实现可参与运行时选择。
    virtual int value() const { return 10; }

    // 没有 virtual：经 Base 访问时仍然选择 Base 的这个函数。
    int fixed() const { return 1; }

    // 本例有虚析构，销毁主题在第 05 篇独立展开。
    virtual ~Base() {
        // 此时 Derived 析构阶段已结束，调用选回 Base 层。
        assert(value() == 10);
        std::cout << "Base destructor: 10\n";
    }
};

class Derived : public Base {
public:
    // 进入 Derived 构造函数体时，本层实现已参与分派。
    Derived() { assert(value() == 20); }

    // override 要求编译器核对它确实覆盖了某个基类虚函数。
    int value() const override { return 20; }

    // 与 Base::fixed 同名，但 Base::fixed 不是虚函数。
    int fixed() const { return 2; }

    ~Derived() override {
        // 在 Derived 析构函数体内，仍选择 Derived 层的实现。
        assert(value() == 20);
        std::cout << "Derived destructor: 20\n";
    }
};

// 参数的静态类型是 const Base&，不决定对象必须是独立的 Base。
int read_virtual(const Base& object) {
    return object.value();
}

int read_nonvirtual(const Base& object) {
    return object.fixed();
}

int main() {
    // 内部块使析构时机出现在最终提示之前，方便阅读日志。
    {
        Derived derived;

        // ref 和 ptr 都借用 derived 内部的 Base 子对象，没有切片。
        Base& ref = derived;
        Base* ptr = &derived;

        assert(derived.value() == 20);
        assert(ref.value() == 20);
        assert(ptr->value() == 20);
        assert(read_virtual(derived) == 20);

        assert(derived.fixed() == 2);
        assert(ref.fixed() == 1);
        assert(ptr->fixed() == 1);
        assert(read_nonvirtual(derived) == 1);

        // 显式写出 Base::，这次调用抑制虚分派，直接选 Base 实现。
        assert(ref.Base::value() == 10);
        assert(ptr->Base::value() == 10);

        // 改变访问类型没有改变原对象；回到 derived 仍得到 20。
        assert(derived.value() == 20);
        std::cout << "virtual: 20, nonvirtual via Base: 1\n";
        std::cout << "qualified Base::value: 10\n";
    }
    std::cout << "virtual dispatch checks passed\n";
}
```
