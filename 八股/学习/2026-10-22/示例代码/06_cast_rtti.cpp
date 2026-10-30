// ============================================================================
// 02_cast_rtti.cpp
// 目的：把 C++ 四种显式类型转换各用一次，并看清 dynamic_cast 与 RTTI 的边界。
// 前置：知道基类/派生类指针的区别，知道虚函数让类型在运行时仍可辨认。
// 程序步骤：
//   1. static_cast：相关类型之间的转换，编译器只查“关系”，不查运行时真假；
//   2. dynamic_cast：多态类型的安全下行转换，指针失败得 nullptr，引用失败抛异常；
//   3. typeid：读运行时类型信息；
//   4. const_cast：只用于“对象本来不是 const，只是被 const 引用看着”的场景；
//   5. reinterpret_cast：指针与整数、无关指针类型之间的位模式转换。
// 观察点：
//   - dynamic_cast 只对“多态类型”（含虚函数）有效，对普通结构体直接用会编译失败；
//   - static_cast 下行转换在对象类型不符时是未定义行为，程序不会替你检查；
//   - RTTI 可以被编译选项关掉（如 -fno-rtti），依赖它的代码就无法编译。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 02_cast_rtti.cpp -o 02_cast_rtti
// ============================================================================

#include <cstdint>    // uintptr_t
#include <iostream>   // std::cout
#include <typeinfo>   // typeid、std::bad_cast

// 多态基类：有虚函数，运行时才能判断“这个 Base* 到底指向谁”
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;   // 纯虚函数：Shape 不能被直接实例化
};

class Circle : public Shape {
public:
    explicit Circle(double r) : radius_(r) {}   // explicit：禁止 double 隐式变成 Circle
    double area() const override { return 3.1415926 * radius_ * radius_; }
private:
    double radius_;
};

class Square : public Shape {
public:
    explicit Square(double side) : side_(side) {}
    double area() const override { return side_ * side_; }
private:
    double side_;
};

// 一段“老接口”：参数不是 const，但我们手里的对象其实是可写的。
void legacy_print(char* text) { std::cout << "legacy_print: " << text << "\n"; }

int main() {
    std::cout << "=== 1. static_cast：编译期相关类型转换 ===\n";
    double ratio = 3.9;
    int truncated = static_cast<int>(ratio);   // 浮点转整数：截断，不是四舍五入
    std::cout << "static_cast<int>(3.9) = " << truncated << "\n";

    Circle circle(2.0);
    Shape* shape = &circle;                    // 派生类指针 → 基类指针：隐式就能转
    Circle* back = static_cast<Circle*>(shape); // 基类 → 派生：编译器相信你，不做运行时检查
    std::cout << "static_cast 回来后的面积 = " << back->area() << "\n";
    std::cout << "提示：若 shape 实际指向 Square，上面这次转换就是未定义行为\n";

    std::cout << "\n=== 2. dynamic_cast：运行时检查类型 ===\n";
    Square square(3.0);
    Shape* maybe_square = &square;
    Circle* wrong = dynamic_cast<Circle*>(maybe_square);  // 类型不符：返回空指针
    Square* right = dynamic_cast<Square*>(maybe_square);  // 类型相符：返回有效指针
    std::cout << "向下转成 Circle* 的结果：" << (wrong ? "成功" : "失败(nullptr)") << "\n";
    std::cout << "向下转成 Square* 的结果：" << (right ? "成功" : "失败(nullptr)") << "\n";

    try {
        Circle& reference = dynamic_cast<Circle&>(square);  // 引用没有“空”这种值
        std::cout << "引用转换成功，面积 " << reference.area() << "\n";
    } catch (const std::bad_cast& error) {
        // 引用形式转换失败时抛 std::bad_cast，这里会走到
        std::cout << "引用转换失败：" << error.what() << "\n";
    }

    std::cout << "\n=== 3. typeid：读运行时类型 ===\n";
    Shape* shapes[] = {&circle, &square};
    for (Shape* item : shapes) {
        std::cout << "typeid(*item).name() = " << typeid(*item).name()
                  << "，面积 " << item->area() << "\n";
    }
    std::cout << "注意：typeid 对多态类型取的是动态类型，对非多态类型取的是静态类型\n";

    std::cout << "\n=== 4. const_cast：只去掉 const 这一层“视图” ===\n";
    char buffer[] = "mutable text";            // 对象本身不是 const
    const char* view = buffer;
    legacy_print(const_cast<char*>(view));     // 仅仅去掉 const 限定，交给老接口
    std::cout << "若对象本身定义成 const char buffer[]，再去 const 写它属于未定义行为\n";

    std::cout << "\n=== 5. reinterpret_cast：不改变位，只改变解释方式 ===\n";
    Shape* raw = &circle;
    std::uintptr_t address = reinterpret_cast<std::uintptr_t>(raw);
    std::cout << "对象的地址值 = " << std::hex << std::showbase << address << std::dec << std::noshowbase << "\n";
    std::cout << "提示：不要用它做基类/派生类之间的转换，它不会调整指针偏移\n";
    return 0;
}
