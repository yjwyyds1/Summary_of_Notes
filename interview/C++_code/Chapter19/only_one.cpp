//
// Created by 20675 on 2026/10/21.
//
/*
 * ============================================================
 *  LearnCPP 第19章 动态内存分配 - 完整教学示例
 *  把 19.1 ~ 19.5 的核心内容全部放在一个文件里讲解
 * ============================================================
 *
 * 编译建议：g++ -std=c++17 -Wall -Wextra main.cpp -o main
 */

#include <iostream>
#include <cstddef>   // std::size_t
#include <cassert>   // assert
#include <string>

// ============================================================
// 第一部分：19.1  使用 new 和 delete 分配单个变量
// ============================================================

void demo_single_allocation()
{
    std::cout << "\n========== 19.1 单个变量动态分配 ==========\n";

    /*
     * 为什么需要动态分配？
     * 1. 栈空间很小（通常只有几MB），大对象容易栈溢出
     * 2. 很多时候大小要到运行时才知道（用户输入、文件内容）
     * 3. 对象的生命周期需要跨越多个函数，或者由我们主动控制
     *
     * new 做了两件事：
     *   1. 向操作系统（堆）申请一块足够大的内存
     *   2. 在这块内存上构造对象（调用构造函数，如果有的话）
     * 然后返回这块内存的地址。
     */

    // 最基本的写法：分配一个 int，不初始化
    int* ptr1 = new int;          // 申请 4 字节（通常），内容是随机垃圾值
    *ptr1 = 42;                   // 通过指针解引用后赋值
    std::cout << "ptr1 指向的值: " << *ptr1 << '\n';

    // 推荐写法：分配的同时初始化（C++11 统一初始化）
    int* ptr2 = new int{100};     // 直接初始化为 100
    std::cout << "ptr2 指向的值: " << *ptr2 << '\n';

    // 也可以用圆括号初始化（旧风格）
    int* ptr3 = new int(200);

    /*
     * 为什么必须用指针接收 new 的返回值？
     * 因为 new 返回的是堆上内存的地址。
     * 如果不保存这个地址，你就再也找不到这块内存了，
     * 既无法使用，也无法释放 → 内存泄漏。
     */

    // ---------- 释放内存 ----------
    /*
     * delete 做了两件事：
     *   1. 调用对象的析构函数（如果有）
     *   2. 把这块内存还给操作系统（堆）
     *
     * 注意：delete 之后，指针变量本身还在（它是栈上的变量），
     * 只是它指向的内存已经无效了。
     */
    delete ptr1;
    delete ptr2;
    delete ptr3;

    /*
     * 最佳实践：delete 之后立刻把指针设为 nullptr
     * 好处：
     *   1. 防止变成“悬空指针”（dangling pointer）
     *   2. 后面可以安全地判断 if (ptr) 来知道是否已经释放
     *   3. 多次 delete nullptr 是安全的（什么都不做）
     */
    ptr1 = nullptr;
    ptr2 = nullptr;
    ptr3 = nullptr;

    // 演示：new 失败时的处理（极少发生，但生产代码应该考虑）
    int* safe_ptr = new (std::nothrow) int{999};  // 失败时返回 nullptr，不抛异常
    if (safe_ptr == nullptr)
    {
        std::cerr << "内存分配失败！\n";
        // 这里可以做错误处理
    }
    else
    {
        std::cout << "安全分配成功: " << *safe_ptr << '\n';
        delete safe_ptr;
        safe_ptr = nullptr;
    }
}

// ============================================================
// 第二部分：19.2  动态分配数组
// ============================================================

void demo_dynamic_array()
{
    std::cout << "\n========== 19.2 动态数组 ==========\n";

    /*
     * 固定数组的致命缺点：
     *   int arr[100];   // 大小必须在编译时确定
     *
     * 动态数组的优势：
     *   大小可以在运行时由用户输入、计算结果决定
     *   内存来自堆，可以非常大
     */

    std::size_t length = 5;   // 实际项目中通常从 cin 或文件读取

    // 分配一个长度为 length 的 int 数组，并全部初始化为 0
    int* arr = new int[length]{};   // 注意 [] 和后面的 {}

    /*
     * 为什么用 std::size_t？
     * 因为数组长度在语义上永远是非负数，
     * size_t 是无符号类型，更符合“大小”的含义，
     * 也和标准库（vector::size() 等）保持一致。
     */

    // 赋值
    for (std::size_t i = 0; i < length; ++i)
        arr[i] = static_cast<int>(i * 10);

    std::cout << "动态数组内容: ";
    for (std::size_t i = 0; i < length; ++i)
        std::cout << arr[i] << ' ';
    std::cout << '\n';

    /*
     * C++11 之后可以用初始化列表：
     * int* arr2 = new int[5]{1, 2, 3, 4, 5};
     * 注意：没有 = 号，直接写在 [] 后面。
     */

    // ---------- 释放数组 ----------
    /*
     * 关键！必须用 delete[] （带中括号）
     * 如果写成 delete arr;  → 未定义行为（可能崩溃、泄漏、数据损坏）
     *
     * 原因：
     * new[] 会在内存里偷偷记录“我分配了多少个元素”，
     * delete[] 需要这个信息才能正确调用每个元素的析构函数并释放正确大小。
     * 用普通 delete 会把这块内存当成“单个对象”处理，信息对不上。
     */
    delete[] arr;
    arr = nullptr;   // 养成好习惯
}

// ============================================================
// 第三部分：19.3  析构函数 + RAII
// ============================================================

/*
 * 为什么需要析构函数？
 * 当类内部自己 new 了内存，就必须有人负责 delete。
 * 析构函数就是“对象死亡时自动执行的清理函数”。
 *
 * RAII（Resource Acquisition Is Initialization）核心思想：
 *   资源的获取和释放与对象的生命周期绑定。
 *   构造函数里获取资源，析构函数里释放资源。
 *   这样只要对象正确销毁，资源就一定会被释放，极大减少泄漏。
 */
class IntArray
{
private:
    int* m_data{};        // 指向动态分配的数组
    std::size_t m_size{}; // 记录长度（动态数组自己不知道长度）

public:
    // 构造函数：获取资源
    explicit IntArray(std::size_t size)
        : m_size{size}
    {
        assert(size > 0);  // 防止分配 0 长度（实际项目可更友好处理）
        m_data = new int[m_size]{};  // 分配并初始化为 0
        std::cout << "IntArray 构造：分配了 " << m_size << " 个 int\n";
    }

    // 析构函数：释放资源
    ~IntArray()
    {
        delete[] m_data;   // 关键：用 delete[]
        m_data = nullptr;  // 虽然对象马上销毁，但养成好习惯
        std::cout << "IntArray 析构：已释放内存\n";
    }

    // 禁止拷贝（简化示例，避免浅拷贝问题）
    // 实际项目中应该实现深拷贝或使用智能指针
    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;

    // 访问接口
    int& operator[](std::size_t index)
    {
        assert(index < m_size);
        return m_data[index];
    }

    std::size_t size() const { return m_size; }
};

void demo_destructor_and_raii()
{
    std::cout << "\n========== 19.3 析构函数与 RAII ==========\n";

    {
        // 进入这个作用域
        IntArray arr(4);          // 构造函数自动分配内存
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        arr[3] = 40;

        std::cout << "数组内容: ";
        for (std::size_t i = 0; i < arr.size(); ++i)
            std::cout << arr[i] << ' ';
        std::cout << '\n';

        // 离开这个作用域时，arr 自动销毁 → 自动调用析构函数 → 自动释放内存
        // 我们不需要写任何 delete！
    } // ← 这里自动调用 ~IntArray()

    std::cout << "作用域结束后，内存已经安全释放\n";

    /*
     * 好处总结：
     * 1. 程序员不容易忘记释放
     * 2. 即使发生异常（后面章节会讲），栈展开时也会调用析构函数
     * 3. 代码更清晰：资源管理逻辑集中在类内部
     */
}

// ============================================================
// 第四部分：19.4  指针到指针 + 动态二维数组
// ============================================================

void demo_pointer_to_pointer()
{
    std::cout << "\n========== 19.4 指针到指针与动态二维数组 ==========\n";

    /*
     * 普通指针：int*   → 指向一个 int
     * 指针到指针：int** → 指向一个“指向 int 的指针”
     *
     * 最常见用途：实现真正的动态二维数组（行列都在运行时决定）
     */

    const std::size_t rows = 3;
    const std::size_t cols = 4;

    // 第一步：分配“行指针”数组（每一行是一个 int*）
    int** matrix = new int*[rows];

    // 第二步：为每一行分配实际的数据空间
    for (std::size_t r = 0; r < rows; ++r)
    {
        matrix[r] = new int[cols]{};  // 每行 4 个 int，初始化为 0
    }

    // 现在可以像普通二维数组一样使用
    matrix[0][0] = 1;
    matrix[0][1] = 2;
    matrix[1][2] = 3;
    matrix[2][3] = 4;

    std::cout << "动态二维数组内容:\n";
    for (std::size_t r = 0; r < rows; ++r)
    {
        for (std::size_t c = 0; c < cols; ++c)
            std::cout << matrix[r][c] << ' ';
        std::cout << '\n';
    }

    /*
     * 释放顺序必须严格相反！
     * 先释放每一行，再释放“行指针”数组。
     * 顺序反了就会崩溃或泄漏。
     */
    for (std::size_t r = 0; r < rows; ++r)
    {
        delete[] matrix[r];   // 释放第 r 行
        matrix[r] = nullptr;
    }
    delete[] matrix;          // 释放行指针数组
    matrix = nullptr;

    /*
     * 为什么实际项目中很少手写这种代码？
     * 1. 容易写错释放顺序
     * 2. 内存不连续，缓存不友好
     * 3. 有更好的替代：std::vector<std::vector<int>> 或扁平化一维数组
     * 这里写出来是为了让你理解底层原理。
     */
}

// ============================================================
// 第五部分：19.5  空指针（void*）
// ============================================================

void demo_void_pointer()
{
    std::cout << "\n========== 19.5 空指针 void* ==========\n";

    /*
     * void* 是“通用指针”，可以指向任何类型的对象。
     * 它不知道自己指向的到底是什么类型，所以：
     *   - 不能直接解引用
     *   - 不能做指针算术
     *   - 删除前必须先转回原来的类型
     */

    int value = 42;
    float pi = 3.14f;

    void* generic = &value;   // 可以指向 int
    // *generic;              // 错误！不能解引用 void*

    // 必须先转回具体类型才能使用
    int* real_int = static_cast<int*>(generic);
    std::cout << "通过 void* 访问 int: " << *real_int << '\n';

    generic = &pi;            // 现在又指向 float
    float* real_float = static_cast<float*>(generic);
    std::cout << "通过 void* 访问 float: " << *real_float << '\n';

    /*
     * 为什么要有 void*？
     * 历史原因 + 与 C 语言兼容 + 某些底层接口需要“类型擦除”。
     *
     * 现代 C++ 更推荐的替代方案：
     *   - 模板（编译期多态）
     *   - std::any / std::variant（类型安全的类型擦除）
     *   - 基类指针（运行时多态）
     *
     * 结论：能不用 void* 就尽量不用，它会绕过类型检查，容易出错。
     */
}

// ============================================================
// 主函数：按顺序演示所有内容
// ============================================================

int main()
{
    std::cout << "===== LearnCPP 第19章 动态内存分配 完整演示 =====\n";

    demo_single_allocation();      // 19.1
    demo_dynamic_array();          // 19.2
    demo_destructor_and_raii();    // 19.3
    demo_pointer_to_pointer();     // 19.4
    demo_void_pointer();           // 19.5

    std::cout << "\n===== 全部演示结束，程序正常退出 =====\n";
    return 0;
}