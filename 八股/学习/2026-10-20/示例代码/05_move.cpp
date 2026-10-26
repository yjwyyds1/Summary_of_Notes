// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 用计数区分四件常被混在一起的事：复制构造、移动构造、复制赋值、移动赋值。
// std::move 只改变表达式可参与重载选择的形式；真正是否移动，要看随后调用哪个操作。
// 只绑定一个 && 引用不创建新 Packet；有名字的引用表达式仍是左值。
// const 不会被 std::move 去掉，所以从 const Packet 初始化这里的新对象会复制。
// 最后比较可选的命名返回值优化与 C++17 对返回同类型纯右值的保证。
// 正常输出 copies=4 moves=3；该输出在 make_packet 调用之前，后者不影响已经打印的数。

#include <cassert>      // assert 检查内容与次数；成功时不打印，NDEBUG 可关闭运行时断言。
#include <iostream>     // cout 输出复制、移动计数。
#include <memory>       // make_unique/unique_ptr：再用独占所有权验证“绑定”与“转交”的差别。
#include <string>       // string：作为 Packet 内实际拥有字符资源的成员。
#include <type_traits>  // is_lvalue_reference_v：在编译期检查表达式的引用类型。
#include <utility>      // std::move：转换表达式的值类别，不直接执行移动构造或赋值。

struct Packet {
    // 每个 Packet 各有一份 text；这里观察移动/复制对这一成员的操作。
    std::string text;
    // static 成员属于类型，所有 Packet 共用这两个计数，不是每个对象各有一份。
    // inline static 是 C++17 语法，允许在类内定义并初始化它们，无需另写类外定义。
    inline static int copies = 0;
    inline static int moves = 0;

    // explicit 禁止从一个 string 隐式转成 Packet。
    // value 按值接收，随后用 std::move(value) 初始化 text；这是普通构造，不计入下面四项。
    explicit Packet(std::string value) : text(std::move(value)) {}

    // 复制构造：从另一个已存在 Packet 创建“新的 Packet”。
    // const Packet& 只读借用源对象；text(other.text) 复制字符串内容，不改变源内容。
    Packet(const Packet& other) : text(other.text) {
        ++copies;
    }

    // 移动构造：Packet&& 是右值引用，可接收 std::move(非 const 对象) 产生的表达式。
    // other 虽然类型中带 &&，但它有名字，表达式 other.text 仍要再 move 才选择成员移动。
    // noexcept 声明不抛异常；它也是容器选择搬迁方式时会考虑的类型性质。
    Packet(Packet&& other) noexcept : text(std::move(other.text)) {
        ++moves;
    }

    // 复制赋值：左边的 Packet 早已存在，更新它的成员，不是再创建整个对象。
    // operator= 是赋值运算符函数；返回 Packet&，让赋值表达式仍引用左边对象。
    Packet& operator=(const Packet& other) {
        text = other.text;
        ++copies;
        // this 是指向当前对象的指针；*this 解引用得到当前对象，按引用返回而不复制它。
        return *this;
    }

    // 移动赋值：把源成员交给已有目标成员的移动赋值操作；与上面的移动构造区分。
    Packet& operator=(Packet&& other) noexcept {
        text = std::move(other.text);
        ++moves;
        return *this;
    }

    // = default 请求编译器生成正常析构行为；text 仍会自动析构，这不是“什么也不清理”。
    ~Packet() = default;
};

// 按值返回 Packet。local 是有名字的局部对象，这里可能进行 NRVO（命名返回值优化）。
// NRVO 不是该写法在 C++17 下每次必须发生的保证；未发生时可使用返回时的移动规则。
Packet make_packet() {
    Packet local("returned");
    // 直接 return local 表达返回局部值，不需要为了“加速”在这里主动写 std::move。
    return local;
}

struct Direct {
    // default 生成默认构造/析构；delete 明确禁止复制、移动构造。
    Direct() = default;
    Direct(const Direct&) = delete;
    Direct(Direct&&) = delete;
    ~Direct() = default;
};

Direct make_direct() {
    // Direct{} 是同类型纯右值。C++17 可直接构造最终结果，不需要先造临时再移动过去。
    // 所以即使 Direct 的复制和移动构造都被删除，这条返回路径仍可成立。
    return Direct{};
}

int main() {
    // a 创建时使用接收 string 的普通构造函数；copies/moves 仍是 0。
    Packet a("hello");
    Packet b = a;             // 声明新对象 b，a 是左值，调用复制构造，copies = 1。
    Packet c = std::move(a);  // 声明新对象 c，选移动构造，moves = 1。
    // b/c 内容都来自 hello；a 已被移动，不假定它此刻一定为空，也不靠读取它猜实现。
    assert(b.text == "hello");
    assert(c.text == "hello");

    Packet d("old");
    d = b;                   // d 已存在，因此是复制赋值，copies = 2。
    d = std::move(c);         // d 已存在，因此是移动赋值，moves = 2。
    assert(d.text == "hello");

    // && 声明右值引用；这里只让 ref 引用 b，并没有创建另一个 Packet 或调用移动函数。
    Packet&& ref = std::move(b);
    assert(b.text == "hello");
    // decltype((ref)) 检查带括号的表达式 ref 的类型；命名引用表达式是左值，结果为 Packet&。
    // _v 是类型特征的布尔值写法；static_assert 在编译期检查，不是运行时打印或判断。
    static_assert(std::is_lvalue_reference_v<decltype((ref))>);
    Packet e = ref;              // 名字 ref 是左值，选复制构造，copies = 3。
    Packet f = std::move(ref);   // 显式转换后选移动构造，moves = 3；源对象仍是 b。
    assert(e.text == "hello");
    assert(f.text == "hello");

    // const 表示不能通过 fixed 修改这个 Packet；std::move 不会去掉这种限制。
    const Packet fixed("const");
    // std::move(fixed) 是 const Packet&&，不能绑定到需要可修改源的 Packet&& 参数。
    // const Packet& 可以接收它，因此这里选择复制构造，copies = 4，而不是编译失败。
    Packet g = std::move(fixed);
    assert(g.text == "const");
    assert(Packet::copies == 4);
    assert(Packet::moves == 3);
    // :: 用于访问类型的静态成员，<< 依次输出：copies=4 moves=3。
    std::cout << "copies=" << Packet::copies
              << " moves=" << Packet::moves << '\n';

    // auto 推导为 unique_ptr<int>；p 独占一个值为 42 的动态 int。
    auto p = std::make_unique<int>(42);
    // 此处 auto&& 推导出右值引用，仍只是别名；没有第二个 unique_ptr 管理者对象。
    auto&& alias = std::move(p);
    assert(p != nullptr);
    // auto q 没带 &，确实创建新的 unique_ptr；其移动构造才把 p 的所有权转给 q。
    auto q = std::move(alias);
    assert(p == nullptr);
    // 表达式 *q 是解引用；只有在 q 持有有效 int 时才能这样读取。
    assert(*q == 42);

    // 被移动的 a 仍是活着的对象；本例的 string 成员可重新赋值后继续正常使用。
    a.text = "reused";
    assert(a.text == "reused");
    // 不再检查后续的 copies/moves 次数，因为 make_packet 的 NRVO 可影响额外移动次数。
    Packet result = make_packet();
    assert(result.text == "returned");
    // C++17 的同类型纯右值路径直接得到 direct，不调用已经删除的复制/移动构造。
    Direct direct = make_direct();
    // 显式转成 void，表示有意不使用这个对象的值；并没有提前析构 direct。
    (void)direct;
}
