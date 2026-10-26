// =============================================================================
// 第14章「类」—— 全部核心语法 + 设计意图 一次性讲透
// 把下面这份代码完整复制到一个 .cpp 文件，编译运行即可。
// 每一段注释都对应你问的「为什么这样写、有什么好处、设计目的」。
// =============================================================================

#include <cmath>        // std::sqrt
#include <iostream>
#include <string>
#include <string_view>

// -----------------------------------------------------------------------------
// 1. 最基础的 class 语法骨架（对照 struct）
// -----------------------------------------------------------------------------
// class 名字 {
// private:          // 访问说明符：从这里开始的成员只能被「本类的成员函数」访问
//     数据成员;
// public:           // 从这里开始的成员任何人都能访问（只要在作用域内）
//     成员函数;
// };
//
// 为什么默认 private？
//   → 强制「接口与实现分离」。外部代码只能通过 public 接口操作对象，
//     从而保证类不变量永远不被破坏（后面会详细演示）。
//
// 为什么 struct 默认 public？
//   → struct 传统上用来表示「纯数据聚合」，没有不变量需要保护。
//   有不变量、有行为 → 用 class；纯数据袋 → 用 struct。
// -----------------------------------------------------------------------------

class Point2d
{
private:
    // ---------- 数据成员（member variables） ----------
    // 命名惯例：m_ 前缀
    // 好处：一眼就能区分「成员变量」和「局部变量/参数」，避免 value = value 这种迷惑代码。
    // 默认成员初始化器 { 0.0 }：对象出生时如果没人给值，就用这个兜底值。
    // 这比在构造函数体里写 m_x = 0.0 更清晰，也更高效。
    double m_x{ 0.0 };
    double m_y{ 0.0 };

public:
    // ---------- 构造函数（Constructor） ----------
    // 语法：函数名必须与类名完全相同，没有返回类型（连 void 都不写）。
    // 作用：对象「出生」时自动调用，用来建立类不变量（让对象一开始就处于合法状态）。

    // 1）默认构造函数（无参数）
    // = default 的含义：请编译器帮我生成一个「什么都不做、只应用默认成员初始化器」的版本。
    // 好处：
    //   - 保持类型可能是 trivial（可安全 memcpy、可常量初始化）
    //   - 语义清晰：我明确告诉编译器「我要默认构造」
    // 如果写成 Point2d() {}，功能几乎一样，但类型会变成「非平凡」，后续某些优化会受限。
    Point2d() = default;

    // 2）带参数的构造函数 + 成员初始化列表
    // 语法：ClassName(参数列表) : 成员1{值1}, 成员2{值2} { 函数体 }
    //
    // 为什么用初始化列表而不是函数体里赋值？
    //   ① 效率：列表是「直接初始化」，函数体是「先默认初始化再赋值」，多做一次无用功。
    //   ② 必须性：const 成员、引用成员、没有默认构造的成员，只能用列表初始化。
    //   ③ 顺序：实际初始化永远按「成员在类里声明的顺序」进行，与列表书写顺序无关。
    //      所以列表里也按声明顺序写，避免读到未初始化的值。
    //
    // 这里故意不写 explicit（两参数构造一般不会造成意外隐式转换）。
    Point2d(double x, double y)
        : m_x{ x }, m_y{ y }   // 初始化列表：按声明顺序
    {
        // 函数体可以为空；真正的初始化工作已经在列表里完成了。
    }

    // ---------- const 成员函数 ----------
    // 语法：返回类型 函数名(参数) const { ... }
    // const 修饰的是隐藏的 this 指针：this 变成 const Point2d*
    //
    // 含义：我保证不修改对象的任何非 mutable 成员，也不调用任何非 const 成员函数。
    //
    // 为什么必须写 const？
    //   - const 对象 / const 引用 只能调用 const 成员函数。
    //   - 不写 const，你就失去了在 const 场景下使用该函数的能力。
    //   - 口诀：不修改状态的成员函数，一律加 const。
    void print() const
    {
        std::cout << "Point2d(" << m_x << ", " << m_y << ")\n";
        // m_x = 1.0;          // 编译错误：const 成员函数不能修改成员
        // setX(1.0);          // 如果有非 const 的 setX，这里也不能调用
    }

    // 计算到另一个点的距离（演示「同类对象可以互相访问私有成员」）
    // 参数用 const Point2d&：不复制大对象、也不修改对方。
    // 函数本身 const：不修改自己。
    double distanceTo(const Point2d& other) const
    {
        // 注意：other.m_x 能直接访问！
        // 访问控制是「按类」而不是「按对象」。
        // 同一个类的成员函数可以访问任意同类对象的私有成员。
        // 设计目的：保护的是「类的不变量不被类外代码破坏」，而不是对象之间互相防贼。
        const double dx = m_x - other.m_x;
        const double dy = m_y - other.m_y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // ---------- 访问函数（getter / setter） ----------
    // getter：只读，返回值（或 const 引用 / string_view），必须 const。
    double getX() const { return m_x; }
    double getY() const { return m_y; }

    // setter：可以修改，负责守卫不变量（这里简单示例，没有复杂校验）。
    // 如果某个字段「不允许事后修改」，就干脆不提供 setter（不可变类更安全）。
    void setX(double x) { m_x = x; }
    void setY(double y) { m_y = y; }
};

// -----------------------------------------------------------------------------
// 2. 演示「explicit」「委托构造」「类不变量」——用 Fraction
// -----------------------------------------------------------------------------
class Fraction
{
private:
    int m_numerator{ 0 };
    int m_denominator{ 1 };   // 类不变量：分母永远 ≠ 0

public:
    // ---------- explicit 关键字 ----------
    // 语法：explicit ClassName(参数...)
    // 含义：禁止这个构造函数参与「隐式类型转换」。
    //
    // 为什么默认给「只接受一个有效参数」的构造函数加 explicit？
    //   防止荒唐代码静默通过，例如：
    //     f1.multiply(true);   // true 被隐式转成 Fraction{1,1}，语义荒谬
    //   加了 explicit 后，编译器直接报错，把错误从运行期提前到编译期。
    //
    // 例外：如果隐式转换「语义等价且高效」（如 string → string_view），可以不加。
    // 复制/移动构造函数永远不要加 explicit。
    explicit Fraction(int numerator = 0, int denominator = 1)
        : m_numerator{ numerator }
        , m_denominator{ denominator != 0 ? denominator : 1 }  // 守不变量
    {
        // 构造函数是建立不变量的第一道关卡。
        // 对象「生而有效」，后续成员函数只需维护不变量即可。
    }

    // ---------- 委托构造函数（Delegating Constructor） ----------
    // 语法：ClassName(...) : ClassName{实参列表} {}
    // 含义：把初始化工作完全交给另一个构造函数。
    // 规则：初始化列表里「要么委托，要么初始化成员」，两者不能混用。
    // 好处：初始化逻辑集中在一处，修改校验规则只改一个地方。
    // （这里只是演示语法，实际用默认参数更简洁）
    Fraction(int numerator)
        : Fraction{ numerator, 1 }   // 委托给上面的两参版本
    {
    }

    // 成员函数版本的「读取用户输入」
    // 注意：这不是最佳实践（见后面说明），只是为了演示「非成员 → 成员」的转换。
    void getFraction()
    {
        std::cout << "Enter a value for numerator: ";
        std::cin >> m_numerator;
        std::cout << "Enter a value for denominator: ";
        std::cin >> m_denominator;
        if (m_denominator == 0)          // 再次守卫不变量
            m_denominator = 1;
        std::cout << '\n';
    }

    // 乘法：原非成员 multiply(f1, f2) 变成成员后，f1 变成 this
    Fraction multiply(const Fraction& other) const
    {
        // 返回临时对象（匿名对象）。临时对象只活到完整表达式结束。
        return Fraction{
            m_numerator * other.m_numerator,
            m_denominator * other.m_denominator
        };
    }

    void print() const
    {
        std::cout << m_numerator << '/' << m_denominator << '\n';
    }

    // getter（供非成员函数使用）
    int getNumerator() const { return m_numerator; }
    int getDenominator() const { return m_denominator; }
};

// -----------------------------------------------------------------------------
// 3. 为什么有些函数更适合做「非成员」？
// -----------------------------------------------------------------------------
// getFraction / print 如果做成成员，有两个问题：
//   1. 创建对象变成两步：先 Fraction f{}; 再 f.getFraction();
//      非成员可以一步完成：Fraction f{ getFractionFromUser() };
//   2. 它们打印「应用层提示文案」或做格式化输出，不是 Fraction 的「数学本质」。
//      把 I/O 塞进类，类就被污染；换 UI 时还要改类本身。
//
// 判别口诀：
//   「离开这个类型就不完整」的行为 → 成员（乘、加、约分）
//   「沾边但不本质」的行为 → 非成员（I/O、格式化、与外部系统的胶水）
// -----------------------------------------------------------------------------
Fraction getFractionFromUser()
{
    int num{}, den{ 1 };
    std::cout << "Enter a value for numerator: ";
    std::cin >> num;
    std::cout << "Enter a value for denominator: ";
    std::cin >> den;
    std::cout << '\n';
    return Fraction{ num, den };   // 返回临时对象，现代编译器会省略复制（RVO）
}

void printFraction(const Fraction& f)
{
    // 只通过 public 接口访问，不依赖实现细节。
    // 将来 Fraction 内部改成用 double 存储，只要 getter 不变，这个函数不用动。
    std::cout << f.getNumerator() << '/' << f.getDenominator() << '\n';
}

// -----------------------------------------------------------------------------
// 4. 演示「临时对象」「复制构造」「const 对象」等细节
// -----------------------------------------------------------------------------
class Trace
{
public:
    Trace() { std::cout << "  [Trace] 默认构造\n"; }
    Trace(const Trace&) { std::cout << "  [Trace] 复制构造\n"; }  // 有副作用
    ~Trace() { std::cout << "  [Trace] 析构\n"; }
};

Trace makeTrace()
{
    return Trace{};   // 返回临时对象。现代编译器会做复制省略（RVO），
                      // 你几乎看不到「复制构造」的打印。
}

// -----------------------------------------------------------------------------
// 5. 主函数：把上面所有概念跑一遍
// -----------------------------------------------------------------------------
int main()
{
    std::cout << "===== 1. Point2d 基本用法 =====\n";
    Point2d first{};                // 调用默认构造 → (0, 0)
    Point2d second{ 3.0, 4.0 };     // 调用两参构造 → (3, 4)
    // Point2d third{ 4.0 };        // 编译错误：没有单参数构造函数（题目要求）

    first.print();
    second.print();
    std::cout << "Distance: " << first.distanceTo(second) << '\n';  // 5

    std::cout << "\n===== 2. const 对象只能调 const 成员函数 =====\n";
    const Point2d origin{};
    origin.print();                 // OK：print 是 const
    // origin.setX(1.0);            // 编译错误：setX 不是 const

    std::cout << "\n===== 3. Fraction + explicit 的保护作用 =====\n";
    Fraction f1{ 1, 2 };
    Fraction f2{ 3, 4 };
    f1.print();
    // f1.multiply(true);           // 如果构造函数没有 explicit，这里会静默通过并得到荒谬结果。
                                    // 有了 explicit，编译器直接报错 → 把错误提前到编译期。

    std::cout << "乘积: ";
    f1.multiply(f2).print();        // 临时对象上直接调用成员函数

    std::cout << "\n===== 4. 非成员版本的 get / print =====\n";
    Fraction f3{ getFractionFromUser() };  // 一步完成创建+初始化
    std::cout << "你输入的分数: ";
    printFraction(f3);

    std::cout << "\n===== 5. 临时对象与复制省略 =====\n";
    std::cout << "调用 makeTrace()：\n";
    Trace t{ makeTrace() };         // 通常只看到一次「默认构造」，复制被省略
    std::cout << "（上面如果只打印一次构造，就是复制省略在起作用）\n";

    std::cout << "\n===== 6. 访问控制是「按类」不是「按对象」 =====\n";
    // 在 distanceTo 里我们已经看到：同类对象的私有成员可以互相访问。
    // 这是故意设计的：保护的是类的不变量，而不是对象之间的隐私。

    return 0;
}

/*
=============================================================================
快速语法速查表（以后看别人代码时对照）
=============================================================================

1. class 声明
   class 名字 {
   private:
       类型 m_成员{ 默认值 };
   public:
       // 成员函数
   };

2. 构造函数
   名字() = default;                          // 默认构造（推荐）
   名字(参数) : 成员{值}, ... { }             // 初始化列表
   explicit 名字(参数) : ... { }              // 禁止隐式转换
   名字(...) : 名字{实参} { }                 // 委托构造

3. 成员函数
   返回类型 函数名(参数) const { }            // const 成员函数（只读）
   返回类型 函数名(参数) { }                  // 可修改成员

4. 访问说明符
   public:     // 接口（任何人可用）
   private:    // 实现（只有本类成员函数可用）
   // 建议布局：public 在前，private 在后（接口先行）

5. 临时对象
   名字{实参}                                 // 没有名字，活到分号结束
   const 名字& r = 名字{实参};                // 绑定 const 引用可延长寿命

6. 复制构造（编译器默认生成）
   名字(const 名字& other);                   // 逐成员复制
   // 有裸指针/资源时必须自己写深拷贝（第21章「大五」）

=============================================================================
设计目的一句话总结
=============================================================================
这一章所有机制都在服务同一件事：
让对象「生而有效，且只能以合法方式改变」。

- 类不变量     = 目标
- 访问控制     = 手段（挡住类外代码）
- 构造函数     = 出生时建立不变量
- const / explicit = 让编译器替你站岗
- 复制省略     = 让你放心写高可读性代码而不牺牲性能
*/