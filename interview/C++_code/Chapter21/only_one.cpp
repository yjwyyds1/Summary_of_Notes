//
// Created by 20675 on 2026/10/22.
//
/*
 * ============================================================
 *  第 21 章完整示例：运算符重载 + 类型转换 + 复制构造 + 深拷贝
 *  把本章所有知识点都放在一个文件里，带超详细中文注释
 * ============================================================
 *
 * 编译建议：g++ -std=c++17 -Wall -Wextra chapter21_demo.cpp -o demo
 */

#include <iostream>
#include <cassert>
#include <cmath>          // std::trunc, std::round
#include <cstdint>        // std::int16_t, std::int8_t, std::int32_t
#include <utility>        // std::swap（copy-and-swap 用）

// ============================================================
// 一、运算符重载的本质 & 三种形式对比
// ============================================================

/*
 * 运算符重载 = 函数重载的语法糖。
 * 写 a + b 时，编译器其实去找 operator+(a, b) 这个函数。
 * 所以 a + b  和  operator+(a, b)  完全等价。
 *
 * 三种写法：
 * 1. 普通函数（free function）—— 参数对称，左操作数可以不是自己的类
 * 2. 友元函数（friend）—— 能访问私有成员，但破坏一点封装
 * 3. 成员函数 —— 左操作数必须是自己的类（*this），适合修改自身的运算
 */

class Point
{
private:
    int m_x{};
    int m_y{};

public:
    // 普通构造函数
    Point(int x = 0, int y = 0) : m_x{x}, m_y{y} {}

    // 公有访问器（给普通函数用，因为普通函数不能访问私有成员）
    int x() const { return m_x; }
    int y() const { return m_y; }

    // ---------- 形式3：成员函数（一元负号、+= 这种修改自身的最适合） ----------
    // 一元负号：只有一个操作数，让它当 *this 最自然
    // 返回新对象，不修改原对象，所以加 const
    Point operator-() const
    {
        // 为什么返回新对象而不是修改自己？
        // 因为 -a 的语义是“得到 a 的相反数”，不是“把 a 变成相反数”
        return Point{ -m_x, -m_y };
    }

    // 成员函数形式的 +（演示用，实际更推荐普通/友元，因为对称性更好）
    // 注意：成员函数的二元运算符只写 1 个参数，左操作数是 *this
    Point operator+(const Point& other) const
    {
        // 为什么加 const？因为 + 不应该修改左操作数
        return Point{ m_x + other.m_x, m_y + other.m_y };
    }

    // ---------- 形式2：友元函数（能访问私有成员） ----------
    // 减法做成友元：参数对称，两个操作数地位平等
    friend Point operator-(const Point& p1, const Point& p2)
    {
        // 友元可以直接读 m_x、m_y，不用写 getter
        return Point{ p1.m_x - p2.m_x, p1.m_y - p2.m_y };
    }

    // 流输出几乎永远是友元（左操作数是 ostream，你改不了标准库）
    // 返回 ostream& 是为了支持链式：std::cout << a << b;
    friend std::ostream& operator<<(std::ostream& out, const Point& p)
    {
        out << '(' << p.m_x << ", " << p.m_y << ')';
        return out;   // 必须返回引用，否则链式输出会失败
    }
};

// ---------- 形式1：普通函数（只能用公有接口） ----------
// 标量乘法做成普通函数的好处：可以同时支持 Point * int 和 int * Point
Point operator*(const Point& p, int scale)
{
    // 只能通过公有接口访问，封装性最好
    return Point{ p.x() * scale, p.y() * scale };
}

// 对称版本：int * Point（成员函数根本写不出来，因为左操作数不是 Point）
Point operator*(int scale, const Point& p)
{
    return p * scale;   // 直接复用上面的版本
}


// ============================================================
// 二、必须做成成员函数的运算符（语言强制规定）
// ============================================================

/*
 * 以下四个运算符语言规定必须是成员函数：
 *   =   []   ()   ->
 * 写成普通函数或友元会直接编译报错。
 *
 * 原因：它们的语义都是“对左操作数自身动手”，左操作数必须是类对象。
 */

class SimpleArray
{
private:
    int m_data[5]{};

public:
    // 下标运算符：必须返回引用，这样 a[i] = 10 才能当左值
    int& operator[](int index)
    {
        assert(index >= 0 && index < 5);
        return m_data[index];   // 返回引用 → 可以修改
    }

    // 赋值运算符：后面专门讲，这里先占位
    // SimpleArray& operator=(const SimpleArray& other);
};


// ============================================================
// 三、前置/后置 ++ 的经典写法（面试常考）
// ============================================================

class Counter
{
private:
    int m_value{0};

public:
    // 前置 ++ ：先加再返回自己（返回引用，高效，支持链式）
    Counter& operator++()
    {
        ++m_value;
        return *this;           // 返回引用，可以写 ++++c;
    }

    // 后置 ++ ：先返回旧值，再加
    // 那个 int 参数是“哑元”，纯粹用来区分前置/后置，永远不会被用到
    Counter operator++(int)
    {
        Counter old{*this};     // 先保存旧值
        ++m_value;              // 再真正加
        return old;             // 返回旧值的副本（必须按值返回）
    }

    friend std::ostream& operator<<(std::ostream& out, const Counter& c)
    {
        out << c.m_value;
        return out;
    }
};


// ============================================================
// 四、修改自身的二元运算符（+= 等）→ 成员函数 + 返回引用
// ============================================================

class Average
{
private:
    std::int32_t m_total{0};    // 用 int32_t 防止累加溢出（int 在某些平台只有 16 位）
    int          m_numbers{0};

public:
    Average() = default;

    // += 修改自身，所以做成成员函数最干净
    // 返回 Average& 是为了支持链式：(avg += 6) += 10;
    Average& operator+=(std::int32_t num)
    {
        m_total   += num;
        ++m_numbers;
        return *this;           // 关键！必须返回引用，否则第二次 += 作用在临时对象上
    }

    // 输出平均值：需要读私有成员 → friend
    friend std::ostream& operator<<(std::ostream& out, const Average& avg)
    {
        if (avg.m_numbers == 0)
        {
            out << 0;
            return out;
        }
        // 必须 static_cast<double>，否则整数除法 13/2 = 6
        out << static_cast<double>(avg.m_total) / avg.m_numbers;
        return out;
    }
};


// ============================================================
// 五、类型转换：转换构造函数 + 转换函数（operator T）
// ============================================================

/*
 * 两个方向：
 * 1. 别的类型 → 我的类：转换构造函数（单参数构造函数默认可以隐式转换）
 * 2. 我的类 → 别的类型：转换函数 operator T()
 *
 * 强烈建议两边都加 explicit，防止编译器偷偷转换导致二义性。
 */

class FixedPoint2
{
private:
    std::int16_t m_base{};      // 整数部分
    std::int8_t  m_decimal{};   // 小数部分（-99 ~ 99）

public:
    // ---------- 转换构造函数（别的类型 → FixedPoint2） ----------
    // explicit 禁止隐式转换：FixedPoint2 fp = 5.01; 会报错，必须写 FixedPoint2{5.01}
    explicit FixedPoint2(std::int16_t base = 0, std::int8_t decimal = 0)
        : m_base{base}, m_decimal{decimal}
    {
        // 符号统一：只要有一个为负，整个数就为负
        if (m_base < 0 || m_decimal < 0)
        {
            if (m_base > 0)   m_base   = -m_base;
            if (m_decimal > 0) m_decimal = -m_decimal;
        }

        // 小数溢出进位（整数除法 + 取余）
        m_base    += m_decimal / 100;
        m_decimal  = m_decimal % 100;
    }

    // 接受 double 的构造函数（委托构造）
    // 好处：负号处理、进位逻辑只写一遍，不会重复
    explicit FixedPoint2(double d)
        : FixedPoint2(
            static_cast<std::int16_t>(std::trunc(d)),
            static_cast<std::int8_t>(std::round(d * 100) - std::trunc(d) * 100)
          )
    {
        // 空函数体，所有工作都委托给上面的构造函数
    }

    // ---------- 转换函数（FixedPoint2 → double） ----------
    // 语法特点：没有返回类型！operator double 本身就表示“返回 double”
    // explicit 禁止隐式转换，必须写 static_cast<double>(fp)
    explicit operator double() const
    {
        return m_base + static_cast<double>(m_decimal) / 100.0;
    }

    // 相等比较：对称、需要读私有 → friend
    friend bool operator==(const FixedPoint2& a, const FixedPoint2& b)
    {
        return a.m_base == b.m_base && a.m_decimal == b.m_decimal;
    }

    // 一元负号：成员函数
    FixedPoint2 operator-() const
    {
        // 转成 double 取负再转回来，复用已有逻辑
        return FixedPoint2{ -static_cast<double>(*this) };
    }

    // 输出：因为有转换函数，直接转 double 输出即可
    friend std::ostream& operator<<(std::ostream& out, const FixedPoint2& fp)
    {
        out << static_cast<double>(fp);
        return out;
    }
};

// 二元 + ：普通函数（对称），借力转换函数和转换构造
FixedPoint2 operator+(const FixedPoint2& a, const FixedPoint2& b)
{
    // 转成 double 相加，再转回 FixedPoint2，进位逻辑自动处理
    return FixedPoint2{ static_cast<double>(a) + static_cast<double>(b) };
}


// ============================================================
// 六、复制构造 + 赋值运算符 + 深拷贝（最容易出错的部分）
// ============================================================

/*
 * 复制构造函数：用已有对象初始化一个【新对象】
 * 赋值运算符：把已有对象的内容拷贝到【已经存在的对象】上
 *
 * 判断口诀：
 *   新对象诞生 → 复制构造
 *   对象已存在 → 赋值运算符
 *
 * 默认生成的是浅拷贝（只拷指针地址），有动态内存时必须自己写深拷贝。
 * 需要自定义析构/复制构造/赋值 三者之一时，通常三个都要写 → Rule of Three
 */

class IntArray
{
private:
    int  m_length{0};
    int* m_array{nullptr};

public:
    // 构造函数：explicit 防止 IntArray a = 5; 这种意外转换
    explicit IntArray(int length)
        : m_length{length}
    {
        assert(length > 0);
        // {} 值初始化，把数组元素都置 0
        m_array = new int[static_cast<std::size_t>(m_length)]{};
    }

    // ---------- ① 复制构造函数（深拷贝） ----------
    IntArray(const IntArray& other)
        : m_length{other.m_length}
    {
        // 分配【自己的】新内存
        m_array = new int[static_cast<std::size_t>(m_length)]{};
        // 把内容一个个拷过来（而不是只拷指针）
        for (int i = 0; i < m_length; ++i)
            m_array[i] = other.m_array[i];
    }

    // ---------- ② 析构函数 ----------
    ~IntArray()
    {
        delete[] m_array;   // 必须释放，否则内存泄漏
        // 注意：delete[] 对 nullptr 是安全的，所以不用判断
    }

    // ---------- ③ 赋值运算符（深拷贝 + 自赋值检查） ----------
    IntArray& operator=(const IntArray& other)
    {
        // 自赋值检查：a = a; 时如果先 delete 再拷，就读到已释放的内存
        if (this == &other)
            return *this;

        // 先释放自己原来的内存
        delete[] m_array;

        // 再按 other 的大小重新分配
        m_length = other.m_length;
        m_array  = new int[static_cast<std::size_t>(m_length)]{};

        // 深拷贝内容
        for (int i = 0; i < m_length; ++i)
            m_array[i] = other.m_array[i];

        return *this;   // 返回引用支持链式 a = b = c;
    }

    // 更现代的写法（C++11 后推荐）：copy-and-swap
    // 按值传参 → 进函数时已经完成了复制构造（深拷贝）
    // 然后和自己交换，旧资源由 other 的析构带走
    // 天然处理自赋值，而且强异常安全
    /*
    IntArray& operator=(IntArray other)   // 注意：按值传参
    {
        std::swap(m_length, other.m_length);
        std::swap(m_array,  other.m_array);
        return *this;
    }
    */

    // 下标运算符：必须是成员函数，返回引用才能当左值
    int& operator[](int index)
    {
        assert(index >= 0 && index < m_length);
        return m_array[index];
    }

    // 输出
    friend std::ostream& operator<<(std::ostream& out, const IntArray& arr)
    {
        for (int i = 0; i < arr.m_length; ++i)
            out << arr.m_array[i] << ' ';
        return out;
    }
};


// ============================================================
// 七、禁止拷贝的类（= delete）
// ============================================================

/*
 * 有些资源是独占的（文件句柄、互斥锁、unique_ptr），不允许被拷贝。
 * 用 = delete 在编译期直接禁止，比写成 private 不实现更清晰。
 */
class NonCopyable
{
public:
    NonCopyable() = default;

    // 禁止复制构造
    NonCopyable(const NonCopyable&) = delete;

    // 禁止复制赋值
    NonCopyable& operator=(const NonCopyable&) = delete;

    // C++11 后还可以禁止移动（如果需要）
    // NonCopyable(NonCopyable&&) = delete;
    // NonCopyable& operator=(NonCopyable&&) = delete;
};


// ============================================================
// 主函数：把所有概念串起来演示
// ============================================================

int main()
{
    std::cout << "===== 1. 运算符重载三种形式 =====\n";
    Point p1{2, 3};
    Point p2{4, 5};

    std::cout << "p1 + p2 = " << (p1 + p2) << '\n';          // 成员函数
    std::cout << "p1 - p2 = " << (p1 - p2) << '\n';          // 友元
    std::cout << "p1 * 3  = " << (p1 * 3)  << '\n';          // 普通函数
    std::cout << "3 * p1  = " << (3 * p1)  << '\n';          // 对称普通函数
    std::cout << "-p1     = " << (-p1)     << '\n';          // 一元成员函数

    std::cout << "\n===== 2. 前置/后置 ++ =====\n";
    Counter c;
    std::cout << "初始: " << c << '\n';
    std::cout << "前置++后: " << ++c << '\n';              // 先加再返回
    std::cout << "后置++后: " << c++ << '\n';              // 先返回旧值再加
    std::cout << "现在值: " << c << '\n';

    std::cout << "\n===== 3. += 链式调用 =====\n";
    Average avg;
    (avg += 6) += 10;                                   // 全靠返回引用
    std::cout << "平均值: " << avg << '\n';                // 应输出 8

    std::cout << "\n===== 4. 定点数 FixedPoint2 =====\n";
    FixedPoint2 fp1{5.01};                              // 委托构造 + round 处理精度
    FixedPoint2 fp2{1.999};                             // 进位测试
    std::cout << "fp1 = " << fp1 << '\n';
    std::cout << "fp2 = " << fp2 << '\n';
    std::cout << "fp1 + fp2 = " << (fp1 + fp2) << '\n';
    std::cout << "-fp1 = " << (-fp1) << '\n';

    std::cout << "\n===== 5. 深拷贝 IntArray =====\n";
    IntArray a{5};
    a[0] = 5; a[1] = 8; a[2] = 2; a[3] = 3; a[4] = 6;

    IntArray b{a};                                      // 复制构造（深拷贝）
    std::cout << "a = " << a << '\n';
    std::cout << "b = " << b << '\n';

    a[4] = 7;                                           // 改 a 不影响 b
    std::cout << "改 a[4]=7 后:\n";
    std::cout << "a = " << a << '\n';
    std::cout << "b = " << b << '\n';                  // b 末尾仍是 6，证明深拷贝成功

    // 自赋值测试
    auto& ref = a;
    a = ref;                                            // 如果没写自赋值检查会崩溃
    std::cout << "自赋值后 a = " << a << '\n';

    std::cout << "\n===== 6. 禁止拷贝的类 =====\n";
    NonCopyable nc1;
    // NonCopyable nc2 = nc1;                           // 编译错误！= delete 生效
    // nc1 = nc2;                                       // 同样编译错误

    std::cout << "所有演示完成，没有崩溃说明深拷贝和自赋值检查都正确。\n";
    return 0;
}