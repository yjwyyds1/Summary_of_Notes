//
// Created by 20675 on 2026/10/25.
//
// ============================================================
// 第27章 异常处理 —— 完整演示 + 逐行中文注释
// 把本章所有核心点都塞进一个文件，方便你对照理解
// ============================================================

#include <iostream>
#include <stdexcept>   // std::runtime_error / logic_error 等
#include <exception>   // std::exception / std::terminate
#include <string>
#include <utility>     // std::move / std::move_if_noexcept
#include <vector>

// ------------------------------------------------------------
// 1. 为什么需要异常？（对比错误码的缺陷）
// ------------------------------------------------------------
// 老办法：用返回值报告错误
// 问题1：返回值通道被占用（成功值和错误码挤在一起）
// 问题2：调用者可以故意不检查，错误被静默吞掉
// 问题3：错误必须层层转发，中间函数全是样板代码
// 问题4：构造函数没有返回值，根本无法报告错误
//
// 异常的核心思想：把“发现错误”和“处理错误”解耦
// - 发现错误的地方只负责 throw
// - 关心错误的地方用 try/catch 接住
// - 中间函数自动被穿透，不用写任何转发代码
// ------------------------------------------------------------

// ------------------------------------------------------------
// 2. 自定义一个简单异常类（演示可以抛出任意类型，但实践中几乎都继承 std::exception）
// ------------------------------------------------------------
class MyError : public std::runtime_error
{
public:
    // 继承 runtime_error 的好处：
    // 1. 自动拥有 what() 函数
    // 2. 可以被 catch (const std::exception&) 统一接住
    // 3. 可以携带更丰富的信息（这里只是简单示范）
    explicit MyError(const std::string& msg)
        : std::runtime_error(msg)
    {
    }
};

// ------------------------------------------------------------
// 3. Fraction 类 —— 本章测验的核心例子
//    重点：构造函数如何用异常保证“对象永远处于合法状态”
// ------------------------------------------------------------
class Fraction
{
private:
    int m_numerator   = 0;  // 默认成员初始化器（C++11）：兜底默认值
    int m_denominator = 1;  // 分母默认设为 1，避免无意中构造出 0 分母

public:
    // 带默认参数的构造函数，一个函数同时充当默认构造 + 普通构造
    Fraction(int numerator = 0, int denominator = 1)
        : m_numerator{numerator}      // 成员初始化列表：直接初始化，比赋值更高效
        , m_denominator{denominator}
    {
        // ★ 关键设计：在构造函数里校验不变式（invariant）
        // 如果分母为 0，就直接抛异常。
        // 好处：
        // 1. 构造函数没有返回值，只能用异常报告错误
        // 2. 保证“只要对象构造成功，它就一定是合法的”
        // 3. 调用者根本不可能拿到一个非法的 Fraction 对象
        if (m_denominator == 0)
            throw std::runtime_error("Invalid denominator");  // 抛出标准异常
    }

    // 友元声明：让全局的 operator<< 能访问私有成员
    // 为什么不用成员函数？
    // 因为 << 的左操作数是 ostream，无法写成 Fraction 的成员函数
    friend std::ostream& operator<<(std::ostream& out, const Fraction& f);
};

// 全局的输出运算符（标准写法）
std::ostream& operator<<(std::ostream& out, const Fraction& f)
{
    out << f.m_numerator << '/' << f.m_denominator;
    return out;  // 返回流引用，支持链式输出 cout << f << '\n'
}

// ------------------------------------------------------------
// 4. 演示栈展开（stack unwinding）
//    异常会“穿透”中间函数，中间函数完全不用写错误处理代码
// ------------------------------------------------------------
void lowLevel()
{
    std::cout << "  [lowLevel] 准备抛异常\n";
    throw std::runtime_error("来自 lowLevel 的错误");
    // 注意：这行后面的代码永远不会执行
    std::cout << "  [lowLevel] 这行不会打印\n";
}

void midLevel()
{
    std::cout << "  [midLevel] 调用 lowLevel\n";
    // ★ 重点：midLevel 完全不用写任何 try/catch 或错误码转发
    // 异常会直接穿透它，自动销毁它的局部变量
    lowLevel();
    std::cout << "  [midLevel] 这行也不会打印\n";
}

// ------------------------------------------------------------
// 5. 演示重抛（rethrow）与对象切片
// ------------------------------------------------------------
void demoRethrow()
{
    try
    {
        throw MyError("原始 MyError 异常");
    }
    catch (const std::exception& e)  // 按 const 引用捕获（正确做法）
    {
        std::cout << "  捕获到: " << e.what() << '\n';

        // ★ 正确重抛：用裸 throw;
        // 好处：原异常对象完整保留（类型、内容都不丢）
        // throw;

        // ★ 错误写法（演示用，实际千万别写）：
        // throw e;
        // 因为 e 的静态类型是 const std::exception&，
        // 重新抛出会发生对象切片，派生类信息全部丢失！
        // 外层如果想按 MyError 捕获，就接不到了。

        // 这里为了演示，我们故意抛一个新异常
        throw std::logic_error("被转换成 logic_error 后重新抛出");
    }
}

// ------------------------------------------------------------
// 6. 演示函数 try 块（function try block）
//    主要用途：捕获基类/成员对象构造函数抛出的异常
//    普通 try 块罩不住“初始化列表”里的代码
// ------------------------------------------------------------
class Base
{
public:
    Base(int x)
    {
        if (x < 0)
            throw std::runtime_error("Base 构造失败：x 不能为负");
    }
};

class Derived : public Base
{
public:
    // 语法特殊：try 必须写在成员初始化列表之前
    Derived(int x)
    try
        : Base{x}   // 如果 Base 构造抛异常，会进入下面的 catch
    {
        std::cout << "  Derived 构造成功\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "  [函数 try 块] 捕获到: " << e.what() << '\n';
        // ★ 重要：此时对象已经构造失败，this 不可用！
        // 不能访问成员、不能调用成员函数。
        // 通常只能记录日志然后重新抛出（或直接吞掉）。
        throw;  // 原样重抛
    }
};

// ------------------------------------------------------------
// 7. 演示“绝不要从析构函数抛异常”
// ------------------------------------------------------------
class BadDestructor
{
public:
    ~BadDestructor()
    {
        // 千万不要这样写！
        // 如果析构函数在“栈展开”过程中被调用（正在处理另一个异常），
        // 再抛新异常 → 两个异常同时在飞 → 运行时直接调用 std::terminate
        // throw std::runtime_error("析构函数抛异常是致命的");
        std::cout << "  BadDestructor 析构（正确：不抛异常）\n";
    }
};

// ------------------------------------------------------------
// 8. 演示 noexcept 的好处
// ------------------------------------------------------------
class Widget
{
public:
    Widget() = default;

    // ★ 移动构造函数标记为 noexcept
    // 好处1：编译器可以做更多优化
    // 好处2：std::vector 扩容时会优先用移动而不是拷贝（性能大幅提升）
    // 如果移动构造可能抛异常，vector 只能保守地用拷贝，保证强异常安全
    Widget(Widget&&) noexcept
    {
        std::cout << "  Widget 被移动（noexcept）\n";
    }

    // 拷贝构造（作为对比）
    Widget(const Widget&)
    {
        std::cout << "  Widget 被拷贝\n";
    }
};

// ------------------------------------------------------------
// 9. 主函数：把所有知识点串起来演示
// ------------------------------------------------------------
int main()
{
    std::cout << "========== 1. 基本 try/catch + Fraction 例子 ==========\n";
    try
    {
        // 用户输入模拟
        int num = 5;
        int den = 0;   // 故意给 0，触发异常

        Fraction f{num, den};   // 这里会抛 std::runtime_error
        std::cout << "Your fraction is: " << f << '\n';  // 这行不会执行
    }
    catch (const std::exception& e)   // ★ 按 const 引用捕获（标准写法）
    {
        // 为什么用 const 引用？
        // 1. 避免对象切片（如果按值捕获，派生类信息会丢失）
        // 2. 零拷贝开销
        // 3. 能接住所有继承自 std::exception 的异常
        std::cerr << "捕获到异常: " << e.what() << '\n';
    }

    std::cout << "\n========== 2. 栈展开演示（异常穿透中间函数） ==========\n";
    try
    {
        midLevel();   // midLevel 和 lowLevel 都没有写任何错误处理代码
    }
    catch (const std::exception& e)
    {
        std::cout << "在 main 里捕获到: " << e.what() << '\n';
        // 控制权直接从 lowLevel 跳到这里，中间的 midLevel 栈帧被自动销毁
    }

    std::cout << "\n========== 3. 重抛与异常转换 ==========\n";
    try
    {
        demoRethrow();
    }
    catch (const std::exception& e)
    {
        std::cout << "最终在 main 捕获到: " << e.what() << '\n';
    }

    std::cout << "\n========== 4. 函数 try 块演示 ==========\n";
    try
    {
        Derived d{-1};   // 故意让 Base 构造失败
    }
    catch (const std::exception& e)
    {
        std::cout << "main 最终捕获到: " << e.what() << '\n';
    }

    std::cout << "\n========== 5. noexcept 与 vector 扩容的关系 ==========\n";
    {
        std::vector<Widget> v;
        v.emplace_back();          // 先放一个
        std::cout << "准备 reserve(10)，触发扩容……\n";
        v.reserve(10);             // 扩容时，因为 Widget 的移动构造是 noexcept，
                                   // vector 会放心地用移动，而不是保守的拷贝
    }

    std::cout << "\n========== 6. catch 匹配顺序与 catch-all ==========\n";
    try
    {
        throw MyError("测试匹配顺序");
    }
    catch (const MyError& e)          // ★ 派生类必须写在基类前面
    {
        std::cout << "精确匹配到 MyError: " << e.what() << '\n';
    }
    catch (const std::exception& e)   // 基类写在后面
    {
        std::cout << "匹配到 std::exception\n";
    }
    catch (...)                       // catch-all 必须写在最后
    {
        std::cout << "未知异常\n";
    }

    std::cout << "\n========== 7. 析构函数绝不抛异常（正确示范） ==========\n";
    {
        BadDestructor bd;   // 离开作用域时析构，不抛异常
    }

    std::cout << "\n========== 程序正常结束 ==========\n";
    return 0;
}

/*
============================================================
本章核心设计思想总结（方便你以后自己看代码时快速回忆）
============================================================

1. 异常的目的
   - 把错误处理从正常控制流中剥离
   - 让中间函数不用写任何“检查 + 转发”的样板代码

2. throw
   - 立即中断当前函数，开始向外寻找匹配的 catch
   - 可以抛任意类型，但实践中几乎只抛继承自 std::exception 的类

3. try / catch
   - try 监视块内（包括间接调用）的所有代码
   - catch 按书写顺序匹配，第一个匹配的执行
   - 派生类 catch 必须写在基类前面
   - 永远用 const 引用捕获，避免切片和多余拷贝

4. 栈展开
   - 异常会自动销毁中间所有栈帧的局部对象（逆序析构）
   - 这是异常能“穿透”中间函数的根本原因

5. 重抛
   - 用裸 throw; 原样重抛（保留完整类型）
   - 千万别写 throw e;（会发生对象切片）

6. 函数 try 块
   - 唯一能捕获“成员初始化列表”里异常的方式
   - 主要用于派生类构造函数

7. 析构函数
   - 绝对不能抛异常（C++11 起默认 noexcept）
   - 否则在栈展开时直接 terminate

8. noexcept
   - 承诺“我绝不抛异常”
   - 最大好处：让 vector 等容器敢用移动而不是拷贝
   - 违约直接 terminate

9. 使用准则
   - 只用于真正的“异常情况”（文件找不到、内存耗尽、违反不变式……）
   - 不要把异常当普通错误处理机制（用户输入错误更适合返回值）

记住这些设计意图后，以后再看别人写的异常代码，你就能快速判断：
“他为什么这样写？这样写有什么好处？如果不这样写会怎样？”
*/