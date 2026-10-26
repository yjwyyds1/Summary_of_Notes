//
// Created by 20675 on 2026/10/23.
//
// ============================================================
//  第22章完整演示：智能指针与移动语义（超详细注释版）
//  编译建议：g++ -std=c++17 -Wall -Wextra main.cpp -o main
// ============================================================

#include <iostream>
#include <memory>      // unique_ptr / shared_ptr / weak_ptr / make_unique / make_shared
#include <utility>     // std::move
#include <string>
#include <vector>
#include <stdexcept>

// ------------------------------------------------------------
// 1. 演示类：用来观察构造、析构、拷贝、移动的时机
// ------------------------------------------------------------
class Resource {
public:
    Resource(const std::string& name = "无名资源")
        : m_name(name)
    {
        std::cout << "[构造] Resource \"" << m_name << "\" 被创建\n";
    }

    // 拷贝构造：深拷贝语义的典型代表
    Resource(const Resource& other)
        : m_name(other.m_name + "(拷贝)")
    {
        std::cout << "[拷贝构造] 从 \"" << other.m_name
                  << "\" 深拷贝出 \"" << m_name << "\"\n";
    }

    // 移动构造：转移资源，源对象被掏空
    Resource(Resource&& other) noexcept
        : m_name(std::move(other.m_name))   // 字符串自己也支持移动
    {
        other.m_name = "已被移动的空壳";     // 把源对象置为安全空状态
        std::cout << "[移动构造] 资源被偷走，源对象变成 \""
                  << other.m_name << "\"\n";
    }

    // 拷贝赋值
    Resource& operator=(const Resource& other) {
        if (this != &other) {
            m_name = other.m_name + "(拷贝赋值)";
            std::cout << "[拷贝赋值] 现在我是 \"" << m_name << "\"\n";
        }
        return *this;
    }

    // 移动赋值
    Resource& operator=(Resource&& other) noexcept {
        if (this != &other) {
            m_name = std::move(other.m_name);
            other.m_name = "已被移动的空壳";
            std::cout << "[移动赋值] 资源被偷走，源对象变成 \""
                      << other.m_name << "\"\n";
        }
        return *this;
    }

    ~Resource() {
        std::cout << "[析构] Resource \"" << m_name << "\" 被销毁\n";
    }

    void hello() const {
        std::cout << "  → 你好，我是 \"" << m_name << "\"\n";
    }

private:
    std::string m_name;
};


// ------------------------------------------------------------
// 2. 演示「禁用复制语义」的类（独占资源典型写法）
// ------------------------------------------------------------
class ExclusiveHolder {
public:
    ExclusiveHolder(std::unique_ptr<Resource> r) : m_res(std::move(r)) {}

    // 明确删除拷贝，强制只能移动
    ExclusiveHolder(const ExclusiveHolder&) = delete;
    ExclusiveHolder& operator=(const ExclusiveHolder&) = delete;

    // 保留移动（默认即可）
    ExclusiveHolder(ExclusiveHolder&&) = default;
    ExclusiveHolder& operator=(ExclusiveHolder&&) = default;

    void use() {
        if (m_res) m_res->hello();
        else       std::cout << "  → 资源已经被移走了\n";
    }

private:
    std::unique_ptr<Resource> m_res;   // 用 unique_ptr 管理资源
};


// ------------------------------------------------------------
// 3. 循环引用演示用的两个类
// ------------------------------------------------------------
class B;   // 前置声明

class A {
public:
    std::shared_ptr<B> m_b;                 // A 强持有 B
    ~A() { std::cout << "[析构] A 被销毁\n"; }
};

class B {
public:
    // 正确写法：用 weak_ptr 打破循环
    std::weak_ptr<A> m_a;                   // B 只观察 A，不阻止 A 销毁
    // 如果这里写成 std::shared_ptr<A> m_a; 就会产生循环引用导致泄漏

    ~B() { std::cout << "[析构] B 被销毁\n"; }
};


// ============================================================
// 主函数：把所有知识点按顺序演示一遍
// ============================================================
int main() {
    std::cout << "========== 1. 裸指针的问题（RAII 的必要性） ==========\n";
    {
        // 错误示范：裸指针容易泄漏
        // Resource* raw = new Resource("裸指针资源");
        // if (true) return 0;          // 提前返回 → 泄漏
        // throw std::runtime_error(""); // 异常 → 泄漏
        // delete raw;                  // 容易忘记

        // 正确示范：unique_ptr 把资源生命周期绑定到栈对象
        auto up = std::make_unique<Resource>("unique_ptr管理的资源");
        // 无论正常离开、return、还是抛异常，up 析构时都会自动 delete
        // 这就是 RAII：资源获取即初始化，资源释放与对象生命周期绑定
    }
    std::cout << "（离开作用域，资源自动释放）\n\n";


    std::cout << "========== 2. 复制语义 vs 移动语义 ==========\n";
    {
        Resource a("原始对象");

        // 拷贝构造：深拷贝，源对象保持不变
        Resource b = a;                     // 调用拷贝构造
        a.hello();
        b.hello();

        // 移动构造：转移资源，源对象被掏空（合法但未指定状态）
        Resource c = std::move(a);          // 强制把左值 a 变成右值
        a.hello();                          // a 现在是空壳
        c.hello();
    }
    std::cout << "\n";


    std::cout << "========== 3. 左值、右值、右值引用 ==========\n";
    {
        int x = 10;                         // x 是左值（有名字、能取地址）

        int&  lref = x;                     // 左值引用，只能绑左值
        // int&  bad  = 20;                 // 编译错误：左值引用不能绑右值

        const int& clref = 20;              // const 左值引用可以绑右值（延长临时寿命）

        int&& rref = 30;                    // 右值引用，专门绑右值
        // int&& rref2 = x;                 // 编译错误：右值引用不能直接绑左值

        // 关键认知：一旦右值引用被命名，它自己就变成了左值！
        // 所以下面这行需要再 std::move 一次才能继续移动
        int&& rref3 = std::move(rref);
        (void)lref; (void)clref; (void)rref3;  // 避免未使用警告
    }
    std::cout << "\n";


    std::cout << "========== 4. 为什么移动语义以右值为核心 ==========\n";
    {
        Resource src("可移动的源");

        // 情况1：参数是左值 → 只能拷贝（源对象后面可能还要用）
        Resource copy = src;

        // 情况2：参数是右值（临时对象）→ 可以安全移动（反正马上销毁）
        Resource moved = Resource("临时对象");  // 这里会先构造临时，再移动（或直接构造）

        // 情况3：我们明确知道某个左值以后不用了，就用 std::move 把它变成右值
        Resource stolen = std::move(src);
        src.hello();    // 已被掏空
    }
    std::cout << "\n";


    std::cout << "========== 5. 用 = delete 禁用复制（独占资源） ==========\n";
    {
        auto res = std::make_unique<Resource>("独占资源");
        ExclusiveHolder h1(std::move(res));

        // ExclusiveHolder h2 = h1;          // 编译错误！拷贝被删除
        ExclusiveHolder h2 = std::move(h1); // 只能移动
        h1.use();   // 已经被移走
        h2.use();
    }
    std::cout << "\n";


    std::cout << "========== 6. std::unique_ptr（默认首选） ==========\n";
    {
        // 推荐创建方式：make_unique（异常安全 + 一次完成）
        auto p1 = std::make_unique<Resource>("unique资源");

        // 禁止拷贝
        // auto p2 = p1;                    // 编译错误

        // 允许移动（所有权转移）
        auto p2 = std::move(p1);
        if (!p1) std::cout << "p1 现在是空的（所有权已转移）\n";
        p2->hello();

        // unique_ptr 可以隐式转换为 shared_ptr（反向不行）
        std::shared_ptr<Resource> sp = std::move(p2);
        // p2 现在为空，sp 接管了资源
    }
    std::cout << "\n";


    std::cout << "========== 7. std::shared_ptr（共享所有权） ==========\n";
    {
        // 推荐：make_shared（一次内存分配，对象+控制块在一起）
        auto sp1 = std::make_shared<Resource>("shared资源");
        std::cout << "引用计数: " << sp1.use_count() << "\n";  // 1

        {
            auto sp2 = sp1;                 // 拷贝！引用计数 +1
            std::cout << "引用计数: " << sp1.use_count() << "\n";  // 2
            sp2->hello();
        }   // sp2 析构，引用计数 -1

        std::cout << "引用计数: " << sp1.use_count() << "\n";  // 1
    }   // sp1 析构，引用计数归 0 → 资源被销毁
    std::cout << "\n";


    std::cout << "========== 8. 致命错误：两个 shared_ptr 从同一裸指针构造 ==========\n";
    {
        // ===== 错误示范（千万别学）=====
        // Resource* raw = new Resource("危险资源");
        // std::shared_ptr<Resource> s1(raw);   // s1 创建自己的控制块
        // std::shared_ptr<Resource> s2(raw);   // s2 又创建另一个控制块
        // 两个控制块互不知道对方 → 各自析构时都会 delete → double delete 崩溃

        // ===== 正确写法 =====
        auto s1 = std::make_shared<Resource>("正确共享资源");
        auto s2 = s1;                       // 从已有 shared_ptr 拷贝，共享同一个控制块
        std::cout << "引用计数: " << s1.use_count() << "\n";  // 2
    }
    std::cout << "\n";


    std::cout << "========== 9. std::weak_ptr（打破循环引用） ==========\n";
    {
        auto a = std::make_shared<A>();
        auto b = std::make_shared<B>();

        a->m_b = b;                         // A 强持有 B
        b->m_a = a;                         // B 只弱观察 A（不增加引用计数）

        std::cout << "A 的引用计数: " << a.use_count() << "\n";  // 1（只有自己）
        std::cout << "B 的引用计数: " << b.use_count() << "\n";  // 2（a 持有一份）

        // 使用 weak_ptr 前必须先 lock 升级成 shared_ptr
        if (auto locked = b->m_a.lock()) {
            std::cout << "通过 weak_ptr 成功访问到 A\n";
        }
    }   // a、b 离开作用域后，引用计数正常归零，全部销毁（无泄漏）
    std::cout << "\n";


    std::cout << "========== 10. 移动构造加 noexcept 的重要性 ==========\n";
    {
        // 如果移动构造不声明 noexcept，std::vector 扩容时为了异常安全
        // 会放弃移动而改用拷贝，性能大幅下降。
        std::vector<Resource> vec;
        vec.reserve(2);
        vec.emplace_back("元素1");
        vec.emplace_back("元素2");          // 这里如果发生扩容，会优先用移动
    }
    std::cout << "\n";


    std::cout << "========== 11. 返回局部变量时不要手动 std::move ==========\n";
    {
        // 错误示范（会妨碍 RVO/NRVO，反而可能更慢）
        // Resource bad() {
        //     Resource r("局部");
        //     return std::move(r);     // 不要这样写
        // }

        // 正确写法：直接返回，编译器通常能做到零拷贝零移动
        auto good = []() {
            Resource r("局部对象");
            return r;                   // 最佳
        };
        Resource result = good();
    }
    std::cout << "\n";


    std::cout << "========== 全部演示结束 ==========\n";
    return 0;
}