//
// Created by 20675 on 2026/10/25.
//


/**
 * ============================================================
 *  第25章 继承与虚拟函数 —— 全知识点综合演示（超详细注释版）
 * ============================================================
 *
 * 设计目标：
 * 1. 用一个程序把整章所有概念串起来
 * 2. 每一行关键代码都解释“为什么要这样写”
 * 3. 运行后能直观看到正确行为 vs 错误行为
 * 4. 方便你以后独自读代码时快速回忆
 */

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

// ============================================================
// 1. 基类指针/引用可以指向派生类（向上转型）
// ============================================================
/*
 * 为什么需要向上转型？
 *  - 想用一个函数/容器统一处理“所有派生类”
 *  - 这是实现多态的基础
 *
 * 为什么安全？
 *  - 每个派生对象内部都完整包含一个基类子对象
 *  - 向上转型永远不会丢失信息（只是“视野变窄”）
 */
class Animal {
public:
    // 故意先写成非虚，后面再对比
    const char* getName() const { return "Animal"; }

    // 虚析构函数（后面会详细讲为什么必须虚）
    virtual ~Animal() {
        std::cout << "  ~Animal() 被调用\n";
    }
};

class Dog : public Animal {
public:
    const char* getName() const { return "Dog"; }  // 这里只是“隐藏”，不是覆盖
};

// ============================================================
// 2. 没有虚函数时：基类指针/引用只看到基类部分（早绑定）
// ============================================================
/*
 * 为什么会这样？
 *  - 编译器只看指针/引用的“静态类型”
 *  - 在编译期就决定了调用哪个函数（早绑定）
 *  - 运行时虽然对象是 Dog，但调用决策早已定死
 */
void demo_early_binding() {
    std::cout << "\n===== 2. 早绑定演示（没有 virtual） =====\n";
    Dog d;
    Animal& ref = d;          // 向上转型（合法、安全）
    Animal* ptr = &d;

    std::cout << "d.getName()     = " << d.getName() << "\n";   // Dog
    std::cout << "ref.getName()   = " << ref.getName() << "\n"; // Animal（早绑定）
    std::cout << "ptr->getName()  = " << ptr->getName() << "\n"; // Animal（早绑定）
}

// ============================================================
// 3. 虚函数 + 覆盖（override）
// ============================================================
/*
 * 为什么需要虚函数？
 *  - 让“通过基类指针/引用调用时，真正执行派生类版本”
 *  - 实现运行时多态（晚绑定）
 *
 * 覆盖成立的严格条件：
 *  1. 基类函数必须是 virtual
 *  2. 函数签名完全相同（名字 + 参数 + const 等）
 *  3. 返回类型相同（协变返回类型是唯一例外）
 */
class Shape {
public:
    // 纯虚函数 → 抽象类（后面讲）
    virtual std::ostream& print(std::ostream& out) const = 0;

    // 虚析构函数（必须！）
    virtual ~Shape() = default;

    // 友元 operator<<：统一入口，内部调用虚 print
    // 为什么这样设计？
    //  - operator<< 是自由函数，本身不参与虚分派
    //  - 把真正的打印工作交给虚函数 print，就能正确分派
    friend std::ostream& operator<<(std::ostream& out, const Shape& s) {
        return s.print(out);   // 这里发生虚调用！
    }
};

class Circle : public Shape {
private:
    int radius_;
public:
    explicit Circle(int r) : radius_(r) {}

    // 正确覆盖：签名完全一致 + override
    std::ostream& print(std::ostream& out) const override {
        return out << "Circle(radius=" << radius_ << ")";
    }

    int getRadius() const { return radius_; }
};

class Triangle : public Shape {
public:
    std::ostream& print(std::ostream& out) const override {
        return out << "Triangle";
    }
};

// ============================================================
// 4. override 说明符的价值
// ============================================================
/*
 * 为什么一定要写 override？
 *  - 告诉编译器：“我意图覆盖，请帮我检查”
 *  - 如果签名写错、基类没 virtual、拼写错误 → 直接编译错误
 *  - 没有 override 时，错误会变成“隐藏”，运行时行为完全错，极难排查
 *
 * 现代 C++ 规范（Google、C++ Core Guidelines）强制要求所有覆盖都加 override
 */

// ============================================================
// 5. final 说明符
// ============================================================
/*
 * 两种用法：
 *  1. 禁止再覆盖某个虚函数
 *  2. 禁止再继承某个类
 *
 * 好处：
 *  - 设计意图明确（“到此为止”）
 *  - 编译器可以去虚化（devirtualization），性能更好
 */
class FinalBase final {   // 这个类禁止被继承
public:
    virtual void foo() final { }  // 这个函数禁止再被覆盖
};

// class DerivedFromFinal : public FinalBase {}; // 编译错误！

// ============================================================
// 6. 虚析构函数（最容易被忽略的致命坑）
// ============================================================
/*
 * 问题场景：
 *  Base* p = new Derived;
 *  delete p;
 *
 * 如果 Base 析构不是虚的：
 *  1. 只调用 ~Base()，~Derived() 不执行 → 资源泄漏
 *  2. 按 Base 的大小释放 Derived 的内存 → 未定义行为
 *
 * 规则：
 *  - 只要打算通过基类指针 delete 派生对象，基类析构就必须 virtual
 *  - 派生类析构会自动变成虚函数（不需要再写 virtual）
 */
class BaseWithVirtualDtor {
public:
    virtual ~BaseWithVirtualDtor() {
        std::cout << "  ~BaseWithVirtualDtor()\n";
    }
};

class DerivedWithDtor : public BaseWithVirtualDtor {
public:
    ~DerivedWithDtor() override {
        std::cout << "  ~DerivedWithDtor()\n";
    }
};

void demo_virtual_destructor() {
    std::cout << "\n===== 6. 虚析构函数演示 =====\n";
    BaseWithVirtualDtor* p = new DerivedWithDtor;
    delete p;   // 正确顺序：先 ~Derived → 再 ~Base
}

// ============================================================
// 7. 用作用域解析运算符强制调用基类版本
// ============================================================
/*
 * 什么时候需要？
 *  - 派生类覆盖了函数，但想在内部复用基类实现
 *  - 调试时强制走某个版本对比行为
 *
 * 语法：对象.类名::函数()  或  指针->类名::函数()
 */
class BaseScope {
public:
    virtual void work() {
        std::cout << "BaseScope::work()\n";
    }
};

class DerivedScope : public BaseScope {
public:
    void work() override {
        BaseScope::work();          // 强制调用基类版本
        std::cout << "DerivedScope 额外工作\n";
    }
};

// ============================================================
// 8. 虚表（vtable）机制（注释讲解，不直接写汇编）
// ============================================================
/*
 * 编译器如何实现虚函数？
 *
 * 1. 每个含虚函数的类 → 生成一张虚函数表（vtable）
 *    表里存的是函数指针
 *
 * 2. 每个对象内部被插入一个隐藏指针 vptr
 *    指向所属类的 vtable
 *
 * 3. 调用虚函数时：
 *    对象 → vptr → vtable → 对应函数指针 → 调用
 *
 * 代价：
 *  - 每次调用多一次间接寻址
 *  - 每个对象多一个指针大小（64位系统 8 字节）
 *  - 阻止内联
 *
 * 好处：
 *  - 真正的运行时多态
 */

// ============================================================
// 9. 纯虚函数与抽象类
// ============================================================
/*
 * 纯虚函数 = 0
 *  - 类变成抽象类，不能实例化
 *  - 派生类必须覆盖所有纯虚函数，否则自己也还是抽象类
 *
 * 纯虚函数可以有函数体（类外定义），但类依然是抽象的。
 * 函数体只是给派生类提供“可显式调用的默认实现”。
 */
class AbstractShape {
public:
    virtual void draw() const = 0;          // 纯虚
    virtual ~AbstractShape() = default;
};

// ============================================================
// 10. 接口类（Interface Class）
// ============================================================
/*
 * 定义：没有成员变量 + 所有函数都是纯虚 + 虚析构
 * 作用：只描述“契约”，不提供实现（依赖倒置）
 *
 * 好处：
 *  - 函数面向接口编程
 *  - 任何实现该接口的类都能传入
 *  - 测试时可传 mock，生产时传真实实现
 */
class ILogger {
public:
    virtual void log(const std::string& msg) = 0;
    virtual ~ILogger() = default;
};

class ConsoleLogger : public ILogger {
public:
    void log(const std::string& msg) override {
        std::cout << "[Console] " << msg << "\n";
    }
};

// ============================================================
// 11. 虚基类解决菱形继承
// ============================================================
/*
 * 问题（菱形继承）：
 *        Animal
 *       /      \
 *    Bird      Horse
 *       \      /
 *       Pegasus
 *
 * 普通继承会导致 Animal 被复制两份，数据重复、构造两次、歧义。
 *
 * 解法：中间层虚继承
 *  - Animal 只存在一份
 *  - 最派生类必须直接初始化虚基类
 */
class AnimalBase {
public:
    AnimalBase() { std::cout << "  AnimalBase 构造\n"; }
    virtual ~AnimalBase() = default;
};

class Bird : virtual public AnimalBase {   // 虚继承
public:
    Bird() { std::cout << "  Bird 构造\n"; }
};

class Horse : virtual public AnimalBase {  // 虚继承
public:
    Horse() { std::cout << "  Horse 构造\n"; }
};

class Pegasus : public Bird, public Horse {
public:
    // 最派生类必须直接调用虚基类构造函数
    Pegasus() : AnimalBase(), Bird(), Horse() {
        std::cout << "  Pegasus 构造\n";
    }
};

// ============================================================
// 12. 对象切片（Object Slicing）
// ============================================================
/*
 * 是什么：
 *  把派生类对象按值赋给基类对象时，派生类独有部分被“切掉”
 *
 * 为什么会发生：
 *  Base b = d;  这里的 b 是真正的 Base 对象，类型在编译期就确定了
 *
 * 如何避免：
 *  永远用 基类引用 或 基类指针 传递多态对象
 */
void demo_slicing() {
    std::cout << "\n===== 12. 对象切片演示 =====\n";
    Circle c{10};
    Shape& ref = c;                 // 不切片，多态正常
    // Shape s = c;                 // 编译错误！因为 Shape 是抽象类
    // 如果 Shape 不是抽象类，这里会发生切片
}

// ============================================================
// 13. dynamic_cast 下行转换
// ============================================================
/*
 * 什么时候需要：
 *  容器里混装了不同派生类，需要挑出特定类型做特有操作
 *
 * 规则：
 *  - 只对多态类型（有虚函数）有效
 *  - 指针版失败返回 nullptr
 *  - 引用版失败抛 std::bad_cast
 *
 * 设计提醒：
 *  需要频繁 dynamic_cast 往往是设计信号 → 更好把功能做成虚函数
 */
int getLargestRadius(const std::vector<Shape*>& shapes) {
    int largest = 0;
    for (const Shape* s : shapes) {
        // 下行转换：只有 Circle 才有 getRadius()
        if (const Circle* c = dynamic_cast<const Circle*>(s)) {
            largest = std::max(largest, c->getRadius());
        }
    }
    return largest;
}

// ============================================================
// 14. 完整多态容器演示（裸指针 vs unique_ptr）
// ============================================================
void demo_polymorphic_container() {
    std::cout << "\n===== 14. 多态容器 + dynamic_cast + unique_ptr =====\n";

    // 方式1：裸指针（需要手动 delete，易泄漏）
    std::vector<Shape*> raw;
    raw.push_back(new Circle{7});
    raw.push_back(new Triangle{});
    raw.push_back(new Circle{3});

    for (const Shape* s : raw) {
        std::cout << *s << "\n";   // 虚 print 正确分派
    }
    std::cout << "最大半径 = " << getLargestRadius(raw) << "\n";

    for (Shape* s : raw) delete s;  // 必须依赖虚析构！

    // 方式2：unique_ptr（推荐！RAII 自动管理）
    // 注意：initializer_list 对 unique_ptr 不可用（不可拷贝）
    std::vector<std::unique_ptr<Shape>> smart;
    smart.push_back(std::make_unique<Circle>(15));
    smart.push_back(std::make_unique<Triangle>());
    smart.push_back(std::make_unique<Circle>(8));

    for (const auto& up : smart) {
        std::cout << *up << "\n";
    }

    // 找最大半径（注意用 .get() 拿裸指针）
    int maxR = 0;
    for (const auto& up : smart) {
        if (const Circle* c = dynamic_cast<const Circle*>(up.get())) {
            maxR = std::max(maxR, c->getRadius());
        }
    }
    std::cout << "unique_ptr 最大半径 = " << maxR << "\n";

    // 不需要手动 delete！unique_ptr 析构时自动调用虚析构
}

// ============================================================
// main：把所有演示串起来
// ============================================================
int main() {
    std::cout << "========== 第25章 继承与虚拟函数 全知识点演示 ==========\n";

    demo_early_binding();
    demo_virtual_destructor();

    std::cout << "\n===== 7. 作用域解析强制调用基类 =====\n";
    DerivedScope ds;
    ds.work();

    std::cout << "\n===== 11. 虚基类（菱形继承）构造顺序 =====\n";
    Pegasus p;   // 观察构造顺序：AnimalBase 只构造一次

    demo_slicing();
    demo_polymorphic_container();

    std::cout << "\n===== 10. 接口类使用 =====\n";
    ConsoleLogger logger;
    ILogger& log = logger;          // 向上转型到接口
    log.log("系统启动成功");

    std::cout << "\n========== 演示结束 ==========\n";
    return 0;
}