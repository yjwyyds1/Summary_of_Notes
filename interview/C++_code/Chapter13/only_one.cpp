// ============================================================================
// 第13章 核心要点完整可运行演示代码
// 结构体、枚举、聚合、指定初始化器、NSMI、填充、类模板与CTAD
//
// 编译命令（推荐 C++20）：
//   g++ -std=c++20 -Wall -Wextra -o chapter13_demo chapter13_demo.cpp
//   或者 clang++ -std=c++20 -Wall -Wextra -o chapter13_demo chapter13_demo.cpp
//
// 运行：
//   ./chapter13_demo
//
// 建议用调试器（gdb / lldb / VS Code / CLion）单步执行，观察变量值、
// sizeof、内存布局等变化。
// ============================================================================

#include <iostream>
#include <string>
#include <string_view>
#include <cstddef>   // offsetof
#include <type_traits>

// ============================================================================
// 1. 程序定义类型（Program-defined type）与类型定义
// ============================================================================

// 结构体定义本身就是类型定义。必须先定义后使用。
struct Point
{
    int x{};
    int y{};
};

// 类型定义可以放在头文件中被多次包含（只要完全相同），不受 ODR 严格限制。

// ============================================================================
// 2 & 3. 枚举：无作用域 enum vs 作用域 enum class
// ============================================================================

// 无作用域枚举（unscoped enum）——枚举器泄漏到外层作用域，可隐式转 int
enum Color
{
    red,      // 0
    green,    // 1
    blue      // 2
};

// 下面这行如果取消注释会编译错误：red 已在外层作用域定义
// enum Fruit { red, apple };   // 错误：red 重定义

// 作用域枚举（scoped enum / enum class）——推荐！枚举器不泄漏，不隐式转 int
enum class Status : char   // 可指定底层类型，节省空间
{
    ok,        // 0
    error,     // 1
    pending    // 2
};

// ============================================================================
// 4. 结构体与成员访问
// ============================================================================

struct Employee
{
    int id{};
    int age{};
    double salary{};
};

// ============================================================================
// 5 & 6. 聚合初始化 + 指定初始化器（C++20）
// ============================================================================

// Employee 是聚合（纯数据成员、无用户声明构造函数等）

// ============================================================================
// 7. 非静态成员初始化（NSMI / 默认成员初始化器）
// ============================================================================

struct Player
{
    int health{100};               // 默认成员初始化器
    std::string name{"Hero"};
    double x{0.0};
    double y{0.0};
};

// ============================================================================
// 8. 填充（Padding）演示
// ============================================================================

struct Wasteful
{
    char a;        // 1
    // 7 字节填充（典型 64 位系统，double 要求 8 字节对齐）
    double b;      // 8
    char c;        // 1
    // 7 字节尾部填充
    double d;      // 8
};                 // 通常 sizeof = 32

struct Packed
{
    double b;      // 8
    double d;      // 8
    char a;        // 1
    char c;        // 1
    // 6 字节尾部填充（让数组元素对齐）
};                 // 通常 sizeof = 24

struct A
{
    char c;        // 1
    // 3 字节填充
    int i;         // 4
};                 // sizeof 通常 = 8

struct B
{
    int i;         // 4
    char c;        // 1
    // 3 字节尾部填充
};                 // sizeof 通常 = 8

// ============================================================================
// 9. 类模板 + CTAD + 推导指引
// ============================================================================

template <typename T>
struct Triad
{
    T first{};
    T second{};
    T third{};
};

// C++17 需要推导指引；C++20 对聚合通常可省略，但保留以兼容
template <typename T>
Triad(T, T, T) -> Triad<T>;

// ============================================================================
// 10. 测验示例：Monster
// ============================================================================

struct Monster
{
    // 内嵌无作用域枚举（为了简洁；生产代码更推荐 enum class）
    enum Type
    {
        ogre,
        dragon,
        orc,
        giant_spider,
        slime
    };

    Type type{};                 // 默认 ogre (0)
    std::string name{};          // 拥有名字（安全）
    int health{};
};

// 把类型转成字符串（返回 string_view 避免拷贝）
constexpr std::string_view getMonsterTypeString(Monster::Type type)
{
    switch (type)
    {
    case Monster::ogre:         return "Ogre";
    case Monster::dragon:       return "Dragon";
    case Monster::orc:          return "Orc";
    case Monster::giant_spider: return "Giant Spider";
    case Monster::slime:        return "Slime";
    }
    return "Unknown";            // 防御性兜底
}

void printMonster(const Monster& monster)
{
    std::cout << "This " << getMonsterTypeString(monster.type)
              << " is named " << monster.name
              << " and has " << monster.health << " health.\n";
}

// 打印 Triad 的函数模板
template <typename T>
void print(const Triad<T>& t)
{
    std::cout << '[' << t.first << ", " << t.second << ", " << t.third << "]\n";
}

// ============================================================================
// main：把所有概念串起来运行并观察
// ============================================================================

int main()
{
    std::cout << "========== 1. 程序定义类型 ==========\n";
    Point p{10, 20};
    std::cout << "Point p = {" << p.x << ", " << p.y << "}\n\n";

    // -------------------------------------------------------
    std::cout << "========== 2 & 3. 枚举对比 ==========\n";

    Color c = green;                       // 无作用域：可直接写 green
    std::cout << "Color c = green;  值 = " << c << " (隐式转 int)\n";

    // int x = c;                          // 合法但危险
    // Status s = Status::ok;
    // int y = s;                          // 错误！enum class 不能隐式转 int

    Status s = Status::ok;
    std::cout << "Status s = Status::ok;  底层值 = "
              << static_cast<int>(s) << " (必须 static_cast)\n";

    // 作用域隔离演示
    // Color::red 和 Status 的枚举器完全独立
    std::cout << "enum class 的枚举器必须写 Status::ok，不会污染外层作用域\n\n";

    // -------------------------------------------------------
    std::cout << "========== 4. 成员访问 . 与 -> ==========\n";

    Employee joe{1, 34, 60000.0};
    std::cout << "joe.age = " << joe.age << '\n';

    Employee* ptr = &joe;
    ptr->age = 35;                         // 指针用 ->
    std::cout << "ptr->age = " << ptr->age << '\n';

    Employee& ref = joe;
    ref.age = 36;                          // 引用用 .
    std::cout << "ref.age = " << ref.age << "\n\n";

    // -------------------------------------------------------
    std::cout << "========== 5. 聚合初始化 ==========\n";

    Employee e1{1, 34, 60000.0};           // 完整初始化
    Employee e2{2};                        // 其余成员值初始化为 0
    Employee e3{};                         // 全部值初始化

    std::cout << "e1: id=" << e1.id << ", age=" << e1.age << ", salary=" << e1.salary << '\n';
    std::cout << "e2: id=" << e2.id << ", age=" << e2.age << ", salary=" << e2.salary << '\n';
    std::cout << "e3: id=" << e3.id << ", age=" << e3.age << ", salary=" << e3.salary << "\n\n";

    // 顺序错误不会报错！逻辑错误很难查
    // Employee oops{34, 1, 60000.0};     // id 变成 34，age 变成 1

    // -------------------------------------------------------
    std::cout << "========== 6. 指定初始化器（C++20） ==========\n";

    // 必须按声明顺序写设计符
    Employee joe2{.id = 1, .age = 34, .salary = 60000.0};
    std::cout << "joe2 (指定初始化器): id=" << joe2.id
              << ", age=" << joe2.age << ", salary=" << joe2.salary << '\n';

    Employee partial{.id = 99};            // 其余成员用默认值（这里是 0）
    std::cout << "partial: id=" << partial.id
              << ", age=" << partial.age << ", salary=" << partial.salary << "\n\n";

    // 下面这行会编译错误（顺序反了）
    // Employee bad{.age = 34, .id = 1};

    // -------------------------------------------------------
    std::cout << "========== 7. 默认成员初始化器（NSMI） ==========\n";

    Player p1;                             // 全部用默认值
    std::cout << "Player p1: health=" << p1.health
              << ", name=\"" << p1.name << "\", x=" << p1.x << ", y=" << p1.y << '\n';

    Player p2{50, "Bob"};                  // 显式值覆盖默认值
    std::cout << "Player p2: health=" << p2.health
              << ", name=\"" << p2.name << "\", x=" << p2.x << ", y=" << p2.y << "\n\n";

    // -------------------------------------------------------
    std::cout << "========== 8. 填充（Padding）与 sizeof ==========\n";

    std::cout << "sizeof(char)   = " << sizeof(char)   << '\n';
    std::cout << "sizeof(int)    = " << sizeof(int)    << '\n';
    std::cout << "sizeof(double) = " << sizeof(double) << '\n';
    std::cout << "sizeof(A)      = " << sizeof(A)      << "  (char + int，实际有填充)\n";
    std::cout << "sizeof(B)      = " << sizeof(B)      << "  (int + char，尾部填充)\n";
    std::cout << "sizeof(Wasteful) = " << sizeof(Wasteful) << "  (交替排列，浪费大)\n";
    std::cout << "sizeof(Packed)   = " << sizeof(Packed)   << "  (大成员在前，更紧凑)\n";

    std::cout << "offsetof(A, c) = " << offsetof(A, c) << '\n';
    std::cout << "offsetof(A, i) = " << offsetof(A, i) << "  (中间有填充)\n\n";

    // -------------------------------------------------------
    std::cout << "========== 9. 类模板 + CTAD ==========\n";

    Triad t1{1, 2, 3};                     // CTAD → Triad<int>
    print(t1);

    Triad t2{1.2, 3.4, 5.6};               // CTAD → Triad<double>
    print(t2);

    // 显式指定也可以
    Triad<std::string> t3{"a", "b", "c"};
    print(t3);
    std::cout << '\n';

    // -------------------------------------------------------
    std::cout << "========== 10. Monster 示例 ==========\n";

    Monster ogre{Monster::ogre, "Torg", 145};
    Monster slime{Monster::slime, "Blurp", 23};

    printMonster(ogre);
    printMonster(slime);

    // 用指定初始化器更清晰（C++20）
    Monster dragon{.type = Monster::dragon, .name = "Smaug", .health = 500};
    printMonster(dragon);

    std::cout << "\n========== 全部演示结束 ==========\n";
    std::cout << "建议：用调试器在关键断点，单步观察每个变量的值、类型和内存。\n";

    return 0;
}