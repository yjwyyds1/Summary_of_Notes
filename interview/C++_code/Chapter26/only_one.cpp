//
// Created by 20675 on 2026/10/25.
//
/*
 * 第26章：模板总结与测验 —— 完整注释版
 * 目标：把本章所有重要概念放在一个文件里，用注释讲透
 * 含义 / 为什么这样写 / 好处 / 设计目的
 */

#include <iostream>
#include <string>
#include <string_view>
#include <cstring>      // 为了 strcmp（特化示例用）
#include <iomanip>      // 为了 setprecision（特化示例用）

// ============================================================
// 一、模板的本质：用“占位类型”写一次代码
// ============================================================
/*
 * 含义：
 *   模板就是一张“图纸”，里面用 T、S 这样的占位符代替真实类型。
 *   编译器在真正用到的时候，才根据具体类型“复印”出真正的类/函数。
 *
 * 为什么需要：
 *   没有模板时，同样逻辑要对 int、double、char 各写一遍 → 代码重复。
 *   模板让你写一次，编译器自动生成各种版本。
 *
 * 好处：
 *   1. 代码复用（写一次，到处用）
 *   2. 类型安全（比宏强得多）
 *   3. 零运行时开销（实例化后和手写代码一样快）
 *
 * 设计目的：
 *   解决“相同逻辑、不同类型”的重复问题，同时保持 C++ 的静态类型检查。
 */

// ============================================================
// 二、最基础的模板类：Pair1（单类型参数）
// ============================================================
/*
 * 含义：
 *   两个值必须是同一种类型（都是 T）。
 *
 * 为什么这样写：
 *   1. template <typename T> 必须紧贴在 class 前面，告诉编译器“这是模板”。
 *   2. typename 和 class 在这里完全等价，社区习惯用 typename 表示“这只是类型”。
 *   3. 成员用 T m_x{} 的 {} 是值初始化，保证即使没赋值也有确定值（int→0，指针→nullptr）。
 *   4. 构造函数参数用 const T&：避免大对象拷贝，同时保证不修改实参。
 *   5. 提供 const 和非 const 两套 first()/second()：
 *        - 非 const 对象可以修改
 *        - const 对象只能读
 *      如果没有 const 版本，const Pair1 就调不了 first()。
 *   6. 返回 T& / const T& 而不是 T：避免每次取值都拷贝。
 *
 * 好处：
 *   - 类型安全：不能把 double 塞进 Pair1<int>
 *   - 支持 const 正确性
 *   - 成员初始化安全（{}）
 *
 * 设计目的：
 *   演示最简单的类模板 + const 重载的正确写法。
 */
template <typename T>
class Pair1
{
private:
    T m_x{};   // 值初始化：保证有确定值
    T m_y{};

public:
    // 构造函数：const 引用传参，避免拷贝
    Pair1(const T& x, const T& y)
        : m_x{ x }, m_y{ y }   // 成员初始化列表比赋值更高效
    {
    }

    // 非 const 版本：返回可读写引用
    T& first()  { return m_x; }
    T& second() { return m_y; }

    // const 版本：返回只读引用（给 const 对象用）
    const T& first()  const { return m_x; }
    const T& second() const { return m_y; }
};

// ============================================================
// 三、多类型参数模板：Pair（两个值可以不同类型）
// ============================================================
/*
 * 含义：
 *   第一个值用 T，第二个值用 S，两者可以完全不同。
 *
 * 为什么改名叫 Pair 而不是继续叫 Pair1：
 *   C++ 不允许类像函数那样按模板参数个数“重载”同名。
 *   如果都叫 Pair，编译器会报“重定义”。
 *   函数模板可以按参数个数重载，类模板不行。
 *
 * 为什么这样写：
 *   1. template <typename T, typename S> 用逗号分隔多个参数。
 *   2. first() 返回 T&，second() 返回 S& —— 返回类型跟随各自的模板参数。
 *   3. 仍然提供 const / 非 const 两套重载。
 *
 * 好处：
 *   更灵活，可以存“int + double”、“string + int”等任意组合。
 *
 * 设计目的：
 *   演示多模板参数的写法，以及“类不能按参数个数重载同名”这个规则。
 */
template <typename T, typename S>
class Pair
{
private:
    T m_x{};
    S m_y{};

public:
    Pair(const T& x, const S& y)
        : m_x{ x }, m_y{ y }
    {
    }

    T& first()  { return m_x; }
    S& second() { return m_y; }

    const T& first()  const { return m_x; }
    const S& second() const { return m_y; }
};

// ============================================================
// 四、继承 + 固定部分类型：StringValuePair
// ============================================================
/*
 * 含义：
 *   第一个值永远是 std::string，第二个值由用户指定（S）。
 *
 * 为什么用继承而不是重新写一个类：
 *   复用已有的 Pair 代码，只需要加一层“构造函数适配”。
 *
 * 为什么这样写：
 *   1. template <typename S> 只剩一个参数，因为第一个类型已经固定。
 *   2. : public Pair<std::string, S>
 *        - 必须显式写出模板参数（不能写 public Pair 或 public Pair<>）
 *        - 这是“实例化基类”，不是真正的部分特化。
 *   3. 构造函数用 std::string_view 收 key：
 *        - 字符串字面量可以直接传，不分配堆内存。
 *   4. static_cast<std::string>(key) 必须写：
 *        - std::string 从 string_view 的构造函数是 explicit 的。
 *        - 有开销的转换（堆分配）必须显式，不能悄悄发生。
 *        - 这是 C++ 的设计哲学：有成本的操作必须在代码里看得见。
 *
 * 好处：
 *   - 入口宽松（可以传 const char*、string、string_view）
 *   - 存储安全（内部存真正的 string，不会悬垂）
 *   - 复用 Pair 的所有功能（first/second 等）
 *
 * 设计目的：
 *   演示如何通过继承“固定部分类型”，以及 explicit 转换的必要性。
 *
 * 注意（扩展）：
 *   如果在派生类内部直接写 first()，编译器可能找不到（模板基类查找问题）。
 *   正确写法是 this->first()。本例中是在 main 里调用，所以没问题。
 */
template <typename S>
class StringValuePair : public Pair<std::string, S>
{
public:
    StringValuePair(std::string_view key, const S& value)
        : Pair<std::string, S>{ static_cast<std::string>(key), value }
    {
    }
};

// ============================================================
// 五、全特化示例（函数 + 类）
// ============================================================
/*
 * 含义：
 *   对某个具体类型给出完全不同的实现。
 *
 * 为什么需要：
 *   通用模板对所有类型一视同仁，但某些类型需要特殊照顾。
 *   例如 const char* 想按字符串比较，而不是按指针地址比较。
 *
 * 为什么这样写：
 *   template <> 表示“全特化”，参数表是空的，签名里全部是具体类型。
 *
 * 好处：
 *   让通用逻辑 + 特殊逻辑可以共存，编译器优先选最匹配的特化版本。
 *
 * 设计目的：
 *   演示“给特定类型换实现”的能力。
 *
 * 重要限制：
 *   函数模板不能部分特化（只能全特化）。
 *   想做“部分定制”时，函数应该用重载，而不是特化。
 */

// 通用 max 模板
template <typename T>
T myMax(T a, T b)
{
    return (a > b) ? a : b;
}

// 全特化：专门处理 const char*（按字符串内容比较）
template <>
const char* myMax(const char* a, const char* b)
{
    return (std::strcmp(a, b) > 0) ? a : b;
}

// 通用 Storage 类模板
template <typename T>
class Storage
{
private:
    T m_value;

public:
    Storage(const T& value) : m_value{ value } {}

    void print() const
    {
        std::cout << m_value << '\n';
    }
};

// 全特化：专门处理 double，打印时保留两位小数
template <>
class Storage<double>
{
private:
    double m_value;

public:
    Storage(double value) : m_value{ value } {}

    void print() const
    {
        std::cout << std::fixed << std::setprecision(2) << m_value << '\n';
    }
};

// ============================================================
// 六、非类型模板参数示例（编译期常量）
// ============================================================
/*
 * 含义：
 *   模板参数不一定是类型，还可以是编译期已知的值（通常是 int）。
 *
 * 为什么这样写：
 *   Array 的大小必须在编译期确定，所以用非类型参数 N。
 *   Array<int, 5> 和 Array<int, 6> 是两个完全不同的类型。
 *
 * 好处：
 *   - 大小在编译期就确定 → 可以放在栈上，没有堆分配开销
 *   - 类型安全（大小不同的数组不能互相赋值）
 *
 * 设计目的：
 *   演示 std::array 的核心思想。
 *   对比 std::vector（大小是运行期信息，存在成员变量里）。
 */
template <typename T, int N>
class FixedArray
{
private:
    T m_data[N];

public:
    T& operator[](int index) { return m_data[index]; }
    const T& operator[](int index) const { return m_data[index]; }

    int size() const { return N; }   // 返回编译期常量
};

// ============================================================
// 七、主函数：把所有概念串起来演示
// ============================================================
int main()
{
    std::cout << "===== 1. Pair1（单类型） =====\n";
    Pair1<int> p1{ 5, 8 };
    std::cout << "Pair1: " << p1.first() << ' ' << p1.second() << '\n';

    const Pair1<double> p2{ 2.3, 4.5 };   // const 对象 → 调用 const 版本
    std::cout << "Pair1: " << p2.first() << ' ' << p2.second() << '\n';

    std::cout << "\n===== 2. Pair（双类型） =====\n";
    Pair<int, double> p3{ 5, 6.7 };
    std::cout << "Pair: " << p3.first() << ' ' << p3.second() << '\n';

    const Pair<double, int> p4{ 2.3, 4 };
    std::cout << "Pair: " << p4.first() << ' ' << p4.second() << '\n';

    std::cout << "\n===== 3. StringValuePair（继承 + 固定首类型） =====\n";
    StringValuePair<int> svp{ "Hello", 5 };
    std::cout << "StringValuePair: " << svp.first() << ' ' << svp.second() << '\n';

    std::cout << "\n===== 4. 函数全特化 =====\n";
    std::cout << "myMax(3, 7) = " << myMax(3, 7) << '\n';
    std::cout << "myMax(\"apple\", \"banana\") = " << myMax("apple", "banana") << '\n';

    std::cout << "\n===== 5. 类全特化 =====\n";
    Storage<int> s1{ 42 };
    s1.print();                     // 普通版本

    Storage<double> s2{ 3.14159 };
    s2.print();                     // 特化版本，保留两位小数

    std::cout << "\n===== 6. 非类型模板参数 =====\n";
    FixedArray<int, 5> arr;
    for (int i = 0; i < arr.size(); ++i)
        arr[i] = i * 10;

    std::cout << "FixedArray size = " << arr.size() << ", 内容: ";
    for (int i = 0; i < arr.size(); ++i)
        std::cout << arr[i] << ' ';
    std::cout << '\n';

    return 0;
}