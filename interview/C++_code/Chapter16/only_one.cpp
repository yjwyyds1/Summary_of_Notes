// ============================================================
// 第 16 章 完整教学示例：容器、std::vector 与数组
// 目标：让你以后看到类似代码时，能独立理解「为什么这样写」
// ============================================================

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <cassert>
#include <utility>      // std::pair, std::move
#include <cstddef>      // std::size_t
#include <limits>       // 输入错误处理用

// ------------------------------------------------------------
// 1. 容器是什么？为什么需要它？
// ------------------------------------------------------------
// 单个变量（标量）只能存一个值。
// 容器 = 专门用来存「一组同类型元素」的对象。
// 好处：可以循环处理、动态增减、统一传给函数。
void demo_container_basic()
{
    int score = 90;                          // 标量：只能存一个成绩
    std::vector<int> scores {90, 85, 70};    // 容器：一组成绩

    // size() 返回「元素个数」，不是字节数！
    // sizeof(scores) 通常是 24 字节（三个指针），真正数据在堆上
    std::cout << "元素个数 = " << scores.size() << '\n';
    std::cout << "vector 对象本身大小 = " << sizeof(scores) << " 字节\n";
}

// ------------------------------------------------------------
// 2. 三种数组对比：C 风格 / std::array / std::vector
// ------------------------------------------------------------
// 设计目的：
// - C 风格数组：历史遗留，无边界检查，传参会退化成指针
// - std::array ：固定大小、可 constexpr、有边界检查，现代首选固定长度
// - std::vector：动态大小，日常最常用
void demo_three_arrays()
{
    // C 风格数组（尽量少用）
    int c_arr[5] = {1, 2, 3, 4, 5};
    // 问题：传给函数会退化成指针，丢失长度信息

    // std::array：大小编译期固定，可放在栈上，可 constexpr
    std::array<int, 5> arr {1, 2, 3, 4, 5};
    // 好处：有 .size()、.at()，传参不会丢失长度

    // std::vector：大小可运行时改变（动态数组）
    std::vector<int> vec {1, 2, 3, 4, 5};
    // 好处：可 push_back、resize，真正灵活
}

// ------------------------------------------------------------
// 3. 最重要的初始化陷阱：{5} vs (5)
// ------------------------------------------------------------
// 为什么会有这个坑？
// C++ 的列表初始化（{}）优先级很高，会优先匹配「元素列表」构造函数。
// 直接初始化（()）才会匹配「构造 n 个元素」的构造函数。
void demo_initialization_trap()
{
    // 列表初始化：把 5 当作「一个元素的值」
    std::vector v1 {5};               // 只有 1 个元素，值为 5
    // 等价于 std::vector<int> v1 = {5};

    // 直接初始化：必须显式写元素类型，才能构造 n 个默认值
    std::vector<int> v2(5);           // 5 个元素，全部是 0
    // 如果写成 std::vector v2(5); 会编译错误（CTAD 推不出元素类型）

    // 对比记忆：
    // { ... } → 「这些值」
    // (n)     → 「n 个默认值」
    // (n, val)→ 「n 个 val」

    std::cout << "v1 大小 = " << v1.size() << "（应为 1）\n";
    std::cout << "v2 大小 = " << v2.size() << "（应为 5）\n";
}

// ------------------------------------------------------------
// 4. 访问元素：[] 无检查 vs at() 有检查
// ------------------------------------------------------------
// 设计哲学：C++ 默认信任程序员（追求速度），但提供可选安全手段。
// []  → 不检查边界，越界是未定义行为（UB），速度最快
// at()→ 检查边界，越界抛异常，稍慢但安全
void demo_access()
{
    std::vector<int> v {10, 20, 30};

    // 合法访问
    std::cout << v[0] << ' ' << v.at(1) << '\n';

    // 危险示例（不要在生产代码写！）
    // std::cout << v[5];   // 编译通过，运行时 UB（可能崩溃、可能读垃圾）
    // std::cout << v.at(5); // 抛出 std::out_of_range 异常

    // 建议：
    // - 索引确定合法（算法内部）→ 用 []
    // - 索引来自用户或复杂逻辑 → 用 at() 或先自己检查
}

// ------------------------------------------------------------
// 5. 长度相关：size / capacity / resize / reserve
// ------------------------------------------------------------
// size     = 当前有多少个元素（有效索引范围 [0, size())）
// capacity = 已经分配了能装多少个元素的内存（≥ size）
//
// 为什么要分 size 和 capacity？
// 因为重新分配内存（reallocation）非常昂贵：
// 1. 申请更大内存  2. 把所有元素搬过去  3. 释放旧内存
// 所以 vector 会「多分配一点」，减少搬家次数。
void demo_size_capacity()
{
    std::vector<int> v;

    std::cout << "初始 size=" << v.size()
              << " capacity=" << v.capacity() << '\n';

    // reserve：只增加 capacity，不改变 size，不创建元素
    // 用途：提前告诉 vector「我大概要这么多」，避免多次 reallocation
    v.reserve(10);
    std::cout << "reserve(10) 后 size=" << v.size()
              << " capacity=" << v.capacity() << '\n';

    // 错误示范：reserve 后 size 仍为 0，不能用下标写！
    // v[0] = 42;  // UB！

    // resize：真正改变 size，新元素会被值初始化（int 就是 0）
    v.resize(5);   // 现在有 5 个 0
    std::cout << "resize(5) 后 size=" << v.size()
              << " capacity=" << v.capacity() << '\n';

    // 现在可以安全用下标了
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = static_cast<int>(i * 10);

    // shrink_to_fit：请求把多余的 capacity 释放掉（非强制）
    v.shrink_to_fit();
}

// ------------------------------------------------------------
// 6. 传参：永远优先传 const 引用
// ------------------------------------------------------------
// 为什么？
// vector 是「胖对象」：内部有堆上所有元素的拷贝。
// 传值 = 深拷贝整个数组 → 非常昂贵。
// 传 const 引用 = 零拷贝，且承诺不修改。
void print_vector(const std::vector<int>& v)   // 推荐写法
{
    // 用 range-based for + const auto& ：零拷贝、意图清晰、不会误改
    for (const auto& e : v)
        std::cout << e << ' ';
    std::cout << '\n';
}

// 反例（不要这样写，除非你真的需要副本）
void print_vector_by_value(std::vector<int> v) // 每次调用都深拷贝
{
    // ...
}

// ------------------------------------------------------------
// 7. 复制语义 vs 移动语义
// ------------------------------------------------------------
// 复制：生成完整独立副本（O(n)）
// 移动：把资源所有权转移过去（O(1)），原对象被「掏空」
//
// 设计目的：让「临时对象」或「不再需要的对象」能以极低成本转移资源。
void demo_copy_move()
{
    std::vector<int> a {1, 2, 3};

    // 复制构造：b 是 a 的完整副本
    std::vector<int> b {a};
    b[0] = 99;                // 不影响 a

    // 移动构造：c 接管 a 的数据，a 变成空（有效但未指定状态）
    std::vector<int> c {std::move(a)};
    // 此后不要再使用 a（除非重新赋值）

    std::cout << "b[0] = " << b[0] << "（应为 99）\n";
    std::cout << "c 大小 = " << c.size() << "（应为 3）\n";
    std::cout << "a 大小 = " << a.size() << "（通常为 0）\n";
}

// 按值返回可移动类型是安全且高效的
// 现代编译器有 RVO（返回值优化），通常连移动都不发生
std::vector<int> make_vector()
{
    std::vector<int> v {10, 20, 30};
    return v;   // 不会昂贵拷贝
}

// ------------------------------------------------------------
// 8. 遍历的最佳实践
// ------------------------------------------------------------
void demo_traversal()
{
    std::vector<int> v {1, 2, 3, 4, 5};

    // ① 需要索引时
    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << "v[" << i << "] = " << v[i] << '\n';

    // ② 最推荐：range-based for + const auto&
    // 好处：无越界风险、意图清晰、零拷贝、以后改元素类型也安全
    for (const auto& e : v)
        std::cout << e << ' ';
    std::cout << '\n';

    // ③ 需要修改元素时用 auto&
    for (auto& e : v)
        e *= 2;
}

// ------------------------------------------------------------
// 9. 用枚举做索引 + max_ 断言（自文档化 + 防失同步）
// ------------------------------------------------------------
// 为什么这样设计？
// 1. legs[Animal::dog] 比 legs[1] 可读性强太多
// 2. max_animals 同时充当「种类数量」和断言基准
// 3. 以后加新动物却忘改数组，断言立刻爆炸 → 把 bug 消灭在最早
namespace Animal
{
    enum Type
    {
        chicken,
        dog,
        cat,
        max_animals   // 「计数器」枚举器，值为 3
    };
}

void demo_enum_index()
{
    std::vector<int> legs {2, 4, 4};

    // 编译期/运行时检查：数组长度必须和枚举种类一致
    assert(std::size(legs) == Animal::max_animals);

    std::cout << "狗有 " << legs[Animal::dog] << " 条腿\n";
}

// ------------------------------------------------------------
// 10. push_back vs emplace_back
// ------------------------------------------------------------
// push_back：接受「已经构造好的对象」
// emplace_back：接受「构造函数的参数」，直接在容器内存里原地构造
//
// 好处：emplace_back 可以省掉一次临时对象的构造+移动
void demo_push_emplace()
{
    std::vector<std::string> v;

    std::string s = "hello";
    v.push_back(s);              // 已有对象 → 拷贝/移动进去

    v.push_back("world");        // 先构造临时 string，再搬入
    v.emplace_back("world");     // 直接在 vector 内部构造，更高效

    // 经验法则：
    // - 对象已经存在 → push_back（语义更清晰）
    // - 临时构造     → emplace_back
}

// ------------------------------------------------------------
// 11. 综合小例子：找最小/最大索引（对应测验 Q3 思想）
// ------------------------------------------------------------
// 设计要点：
// - 模板 + const 引用 → 支持任意类型、零拷贝
// - 返回 std::pair → 一次返回两个相关值，比输出参数更现代
// - 循环从 1 开始是正确的（0 已作为初始假设）
template <typename T>
std::pair<std::size_t, std::size_t> find_min_max_indices(const std::vector<T>& v)
{
    assert(!v.empty());   // 防御：空 vector 不能访问 v[0]

    std::size_t min_idx = 0;
    std::size_t max_idx = 0;

    for (std::size_t i = 1; i < v.size(); ++i)
    {
        if (v[i] < v[min_idx]) min_idx = i;
        if (v[i] > v[max_idx]) max_idx = i;
    }
    return {min_idx, max_idx};   // C++17 结构化绑定友好
}

// ------------------------------------------------------------
// 12. 主函数：把所有演示串起来
// ------------------------------------------------------------
int main()
{
    std::cout << "===== 1. 容器基础 =====\n";
    demo_container_basic();

    std::cout << "\n===== 2. 三种数组 =====\n";
    demo_three_arrays();

    std::cout << "\n===== 3. 初始化陷阱 =====\n";
    demo_initialization_trap();

    std::cout << "\n===== 4. 访问方式 =====\n";
    demo_access();

    std::cout << "\n===== 5. size / capacity =====\n";
    demo_size_capacity();

    std::cout << "\n===== 6. 传参（const 引用） =====\n";
    std::vector<int> nums {1, 2, 3, 4, 5};
    print_vector(nums);

    std::cout << "\n===== 7. 复制 vs 移动 =====\n";
    demo_copy_move();

    std::cout << "\n===== 8. 遍历 =====\n";
    demo_traversal();

    std::cout << "\n===== 9. 枚举索引 =====\n";
    demo_enum_index();

    std::cout << "\n===== 10. push/emplace =====\n";
    demo_push_emplace();

    std::cout << "\n===== 11. 找最值索引 =====\n";
    auto [min_i, max_i] = find_min_max_indices(nums);
    std::cout << "最小值索引 = " << min_i
              << "，最大值索引 = " << max_i << '\n';

    std::cout << "\n===== 12. 按值返回 =====\n";
    auto result = make_vector();
    print_vector(result);

    return 0;
}