// ============================================================================
// 02_ctor_dtor.cpp
// 目的：把“构造/析构顺序”和“构造析构期间的虚调用只到当前层”跑出来看一眼。
// 前置：知道基类/派生类、虚函数、override 的基本写法。
// 程序步骤：
//   1. Derived 对象构造，观察基类构造 → 成员 → 函数体的顺序；
//   2. 在 Base 构造函数里调用虚函数，观察实际进了哪一层；
//   3. 对象析构，观察逆序，以及析构期间的虚调用；
//   4. 构造途中抛异常，观察哪些析构函数被调用。
// 观察点：
//   - 构造/析构期间不会“掉到派生类”去执行被覆盖的虚函数；
//   - 基类虚析构让 delete base_ptr 能销毁完整对象；
//   - 构造失败时，已完成的基类与成员会被析构，对象自身的析构不执行。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 02_ctor_dtor.cpp -o 02_ctor_dtor
// ============================================================================

#include <iostream>   // std::cout
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string：演示成员构造顺序

// ---------------------------------------------------------------------------
// 基类：构造函数里故意调用一个可被覆盖的虚函数
// ---------------------------------------------------------------------------
class Base {
public:
    Base() {
        std::cout << "  [构造] Base 构造函数体开始\n";
        // 关键观察点：此时对象的动态类型是 Base，不是 Derived
        hook();
        std::cout << "  [构造] Base 构造函数体结束\n";
    }

    // 虚析构：派生类析构会覆盖它，delete 基类指针时才能正确销毁
    virtual ~Base() {
        std::cout << "  [析构] ~Base 函数体开始\n";
        hook();   // 析构期间同样只派发到当前层
        std::cout << "  [析构] ~Base 函数体结束\n";
    }

    virtual void hook() { std::cout << "  Base::hook（基类版本）\n"; }

private:
    int base_member_ = 1;   // 基类成员，在 Base 构造函数体之前已初始化
};

// ---------------------------------------------------------------------------
// 派生类：成员 name_ 在 Derived 构造函数体之前构造
// ---------------------------------------------------------------------------
class Derived : public Base {
public:
    Derived() {
        std::cout << "  [构造] Derived 构造函数体（此时 vptr 已指向 Derived）\n";
        hook();   // 这里才真正调到 Derived::hook
    }

    ~Derived() override {
        std::cout << "  [析构] ~Derived 函数体（成员还没销毁）\n";
        hook();
    }

    void hook() override { std::cout << "  Derived::hook（派生类版本）\n"; }

private:
    std::string name_ = "derived";   // 成员：在 Derived 函数体前构造，在之后析构
};

// ---------------------------------------------------------------------------
// 构造失败演示：成员构造函数抛异常
// ---------------------------------------------------------------------------
class Explosive {
public:
    Explosive() {
        std::cout << "  [成员] Explosive 构造，即将抛异常\n";
        throw std::runtime_error("成员构造失败");
    }
    // 构造函数抛异常时，这个对象不算构造完成，因此它自己的析构函数不会被调用
    ~Explosive() { std::cout << "  [成员] ~Explosive（构造失败时不会执行）\n"; }
};

class Holder {
public:
    Holder() {
        // 成员 explosive_ 先构造；它抛异常时，这一行永远不会执行
        std::cout << "  [构造] Holder 函数体\n";
    }
    ~Holder() { std::cout << "  [析构] ~Holder（构造失败时不会执行）\n"; }
private:
    Explosive explosive_;
};

int main() {
    std::cout << "=== 1. 正常构造与析构的顺序 ===\n";
    {
        Derived object;                       // 作用域结束自动析构
        std::cout << "--- 对象已完整构造，开始普通调用 ---\n";
        Base* as_base = &object;
        as_base->hook();                      // 现在才派发到 Derived
        std::cout << "--- 离开作用域，逆序析构 ---\n";
    }

    std::cout << "\n=== 2. 通过基类指针 delete（靠虚析构）===\n";
    Base* heap_object = new Derived();
    delete heap_object;                       // 若 ~Base 不是虚函数，这里是未定义行为

    std::cout << "\n=== 3. 构造途中抛异常 ===\n";
    try {
        Holder holder;                        // Explosive 抛异常，Holder 构造失败
        std::cout << "不会走到这里\n";
    } catch (const std::exception& error) {
        std::cout << "捕获到异常：" << error.what() << "\n";
    }
    return 0;
}
