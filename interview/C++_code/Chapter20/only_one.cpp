//
// Created by 20675 on 2026/10/22.
//
/*
 * ============================================================
 *  第 20 章 函数进阶 —— 全部知识点集中在一个文件里
 *  详细注释版（中文）—— 兼容 C++11 及以上
 * ============================================================
 *
 *  本文件目标：
 *  1. 把“参数传递 / 返回值 / 函数指针 / 动态内存 / 调用栈 /
 *     递归 / 命令行参数 / 省略号 / lambda / 二分查找”
 *     全部用真实代码演示一遍。
 *  2. 每一行关键代码都告诉你：为什么要这样写、好处是什么、
 *     如果不这样写会出什么问题。
 *  3. 方便你以后独自看代码时，能立刻理解设计意图。
 */

#include <iostream>
#include <cassert>      // assert
#include <cstdlib>      // std::atoi
#include <cstdarg>      // 省略号（va_list）
#include <algorithm>    // std::sort / std::for_each
#include <vector>

// ============================================================
// 一、参数传递的三种方式
// ============================================================

// ---------- 1.1 按值传递（Pass by Value） ----------
// 含义：把实参的值【复制一份】给形参。
// 好处：函数内部怎么改都不会影响外面的变量，天然安全。
// 代价：有拷贝开销（小类型可忽略，大对象很贵）。
void printValue(int x)          // x 是一份全新的拷贝
{
    std::cout << "[按值] 收到的值 = " << x << '\n';
    x = 999;                    // 只改了拷贝，外面看不见
}

// ---------- 1.2 按引用传递（Pass by Reference） ----------
// 含义：形参是实参的【别名】，不发生拷贝。
// 好处：零拷贝 + 可以直接修改外面的变量。
// 设计目的：大对象避免拷贝、需要函数修改实参时必须用。
void addOne(int& x)             // x 就是外面 a 的另一个名字
{
    ++x;                        // 直接改本体
}

// ---------- 1.3 按地址传递（Pass by Address） ----------
// 含义：传的是指针的【拷贝】。通过解引用可以改指向的对象，
//       但改指针本身不会影响调用者的指针。
// 为什么数组必须用它：内置数组传参会退化成指针，长度丢失。
void printArray(const int* array, int length)
{
    // const 的含义：我承诺不修改数组内容
    // 好处：1.语义清晰 2.防止手滑修改 3.能接受 const 实参
    for (int i = 0; i < length; ++i)
        std::cout << array[i] << ' ';
    std::cout << '\n';
}

// 演示“改指针本身无效”
void trySetToNull(int* ptr)
{
    ptr = nullptr;              // 只改了本地这份指针拷贝！
                                // 调用者的指针完全不受影响
}

// 如果真的想让函数改变调用者的指针，必须传“指针的引用”
void reallySetToNull(int*& ptr) // 注意 int*&
{
    ptr = nullptr;              // 现在真正改了调用者的指针
}


// ============================================================
// 二、返回值的三种方式
// ============================================================

// 2.1 返回值（最常用、最安全）
int add(int x, int y)
{
    return x + y;               // 返回一份拷贝，生命周期没问题
}

// 2.2 返回引用 —— 正确示范（返回调用者提供的内存）
// 为什么可以返回引用？因为 array 的生命周期比函数长。
// 设计目的：让调用者可以直接修改数组里的元素（像左值一样）。
int& getLargestElement(int* array, int length)
{
    assert(array);              // 防御性编程：空指针立刻报错
    int maxIndex = 0;
    for (int i = 1; i < length; ++i)
        if (array[i] > array[maxIndex])
            maxIndex = i;
    return array[maxIndex];     // 返回的是调用者数组的元素引用
}

// 2.3 返回引用 —— 错误示范（会导致悬空引用）
// 下面这段代码故意写错，用来告诉你为什么不能返回局部变量的引用。
/*
int& doSomethingWrong()
{
    int local = 42;
    return local;               // ❌ 函数结束 local 就销毁了
                                // 返回的引用指向已死亡的内存 → 未定义行为
}
*/


// ============================================================
// 三、函数指针
// ============================================================

// 普通函数
int multiply(int a, int b) { return a * b; }
int subtract(int a, int b) { return a - b; }

// 用 using 给函数指针起别名（C++11 推荐写法）
// 含义：ArithmeticFcn 是一种“指向 接受两个int、返回int 的函数”的指针类型
using ArithmeticFcn = int(*)(int, int);

// 接受函数指针作为参数 → 实现回调（行为可定制）
// 设计目的：让算法本身不知道具体怎么算，由调用者注入行为
int arithmetic(int x, int y, ArithmeticFcn fcn)
{
    return fcn(x, y);           // 调用传入的函数
}


// ============================================================
// 四、动态内存（堆）
// ============================================================

// 演示 new / delete 的正确用法
void demoDynamicMemory()
{
    // 在堆上分配 5 个 int，并初始化为 0
    int* heapArray = new int[5]{};

    // 使用 ...
    heapArray[0] = 10;

    // 三条铁律：
    // 1. 谁 new，谁 delete（或明确移交所有权）
    // 2. 数组必须用 delete[]，单个用 delete
    // 3. delete 后立刻置空，防止悬空指针被误用
    delete[] heapArray;
    heapArray = nullptr;

    // 现代 C++ 更推荐 std::vector / std::unique_ptr，让 RAII 自动管理
}


// ============================================================
// 五、调用栈 + 六、递归
// ============================================================

// 递归必须同时满足两个条件：
// 1. 有明确的终止条件（base case）
// 2. 每次递归都更接近终止条件（收敛）
int factorial(int n)
{
    if (n <= 1)                 // 终止条件
        return 1;
    return n * factorial(n - 1); // 向终止条件靠近
}

// 错误示范：没有终止条件 → 最终栈溢出
/*
int sumToWrong(int value)
{
    return value + sumToWrong(value - 1); // 永远不会停
}
*/


// ============================================================
// 七、命令行参数（在 main 里演示）
// ============================================================

// argc = 参数个数（包含程序名）
// argv = 字符串数组，argv[0] 是程序名，argv[1] 开始才是用户参数
// 注意：所有参数永远是 C 风格字符串，哪怕你输入的是数字


// ============================================================
// 八、省略号（Ellipsis）—— 可变参数
// ============================================================

// 三大缺陷：
// 1. 完全没有类型检查
// 2. 不知道传了几个参数（必须自己传 count）
// 3. 容易写出未定义行为
// 现代 C++ 几乎不用它，改用变参模板或 initializer_list
double average(int count, ...)
{
    va_list args;
    va_start(args, count);      // 从 count 后面的参数开始取

    double sum = 0.0;
    for (int i = 0; i < count; ++i)
        sum += va_arg(args, double); // 必须自己写对类型

    va_end(args);
    return sum / count;
}


// ============================================================
// 九、Lambda 函数
// ============================================================

// 语法：
// [捕获列表](参数列表) -> 返回类型 { 函数体 }
// 捕获列表决定能访问外层哪些变量：
//   []     不捕获
//   [=]    按值捕获全部
//   [&]    按引用捕获全部
//   [x,&y] 精确控制


// ============================================================
// 十、二分查找（迭代版 + 递归版）
// ============================================================

// 安全计算中点（兼容所有 C++ 版本，防止整数溢出）
// 为什么不用 (min + max) / 2？因为 min+max 可能溢出
inline int safeMidpoint(int min, int max)
{
    return min + (max - min) / 2;
}

// 迭代版 —— 工程上更推荐（无栈帧开销、无栈溢出风险）
int binarySearchIterative(const int* array, int target, int min, int max)
{
    assert(array);              // 防御：空指针立刻失败

    while (min <= max)          // 区间为空时退出
    {
        int mid = safeMidpoint(min, max);

        if (array[mid] > target)
            max = mid - 1;      // 必须 -1，否则可能死循环
        else if (array[mid] < target)
            min = mid + 1;
        else
            return mid;         // 找到
    }
    return -1;                  // 未找到（约定用 -1）
}

// 递归版 —— 思路与算法描述更接近，适合教学
int binarySearchRecursive(const int* array, int target, int min, int max)
{
    if (min > max)              // 终止条件1：区间空
        return -1;

    int mid = safeMidpoint(min, max);

    if (array[mid] > target)
        return binarySearchRecursive(array, target, min, mid - 1);
    else if (array[mid] < target)
        return binarySearchRecursive(array, target, mid + 1, max);
    else
        return mid;             // 终止条件2：命中
}


// ============================================================
// 主函数 —— 把所有知识点串起来演示
// ============================================================
int main(int argc, char* argv[])
{
    std::cout << "===== 1. 参数传递演示 =====\n";
    int a = 5;
    printValue(a);              // 按值：a 不会变
    std::cout << "按值后 a = " << a << '\n';

    addOne(a);                  // 按引用：a 被改成 6
    std::cout << "按引用后 a = " << a << '\n';

    int arr[] = {10, 20, 30, 40, 50};
    printArray(arr, 5);         // 按地址 + const

    int* p = arr;
    trySetToNull(p);            // 改指针拷贝无效
    std::cout << "trySetToNull 后 p 是否为空？ " << (p == nullptr ? "是" : "否") << '\n';
    reallySetToNull(p);         // 真正改指针
    std::cout << "reallySetToNull 后 p 是否为空？ " << (p == nullptr ? "是" : "否") << '\n';


    std::cout << "\n===== 2. 返回值演示 =====\n";
    std::cout << "add(3,4) = " << add(3, 4) << '\n';

    int numbers[] = {3, 7, 2, 9, 5};
    getLargestElement(numbers, 5) = 100;   // 返回非const引用，可直接赋值
    std::cout << "修改最大元素后数组：";
    printArray(numbers, 5);


    std::cout << "\n===== 3. 函数指针演示 =====\n";
    std::cout << "arithmetic(5,3,add) = " << arithmetic(5, 3, add) << '\n';
    std::cout << "arithmetic(5,3,multiply) = " << arithmetic(5, 3, multiply) << '\n';
    std::cout << "arithmetic(5,3,subtract) = " << arithmetic(5, 3, subtract) << '\n';


    std::cout << "\n===== 4. 动态内存演示 =====\n";
    demoDynamicMemory();
    std::cout << "动态内存已正确释放\n";


    std::cout << "\n===== 5+6. 递归与调用栈演示 =====\n";
    std::cout << "5! = " << factorial(5) << '\n';
    // 如果写无限递归，最终会栈溢出（调用栈空间有限）


    std::cout << "\n===== 7. 命令行参数演示 =====\n";
    if (argc >= 2)
    {
        // argv[1] 永远是 C 字符串，必须转换
        int age = std::atoi(argv[1]);
        std::cout << "你传入的年龄是：" << age << '\n';
    }
    else
    {
        std::cout << "请用命令行传入一个年龄，例如：./a.out 25\n";
    }


    std::cout << "\n===== 8. 省略号演示 =====\n";
    std::cout << "average(3, 1.0, 2.0, 3.0) = " << average(3, 1.0, 2.0, 3.0) << '\n';


    std::cout << "\n===== 9. Lambda 演示 =====\n";
    std::vector<int> v = {4, 1, 3, 5, 2};

    // 用 lambda 自定义排序规则（降序）
    std::sort(v.begin(), v.end(),
              [](int a, int b) { return a > b; });

    std::cout << "降序排序后：";
    std::for_each(v.begin(), v.end(),
                  [](int x) { std::cout << x << ' '; });
    std::cout << '\n';

    // 捕获外层变量的例子
    int threshold = 3;
    auto greaterThan = [threshold](int x) { return x > threshold; };
    std::cout << "4 是否大于 3？ " << greaterThan(4) << '\n';


    std::cout << "\n===== 10. 二分查找演示 =====\n";
    int sorted[] = {1, 3, 5, 7, 9, 11, 13, 15};
    int target = 7;
    int idx1 = binarySearchIterative(sorted, target, 0, 7);
    int idx2 = binarySearchRecursive(sorted, target, 0, 7);
    std::cout << "迭代版找到 " << target << " 的索引：" << idx1 << '\n';
    std::cout << "递归版找到 " << target << " 的索引：" << idx2 << '\n';

    std::cout << "\n===== 全部演示结束 =====\n";
    return 0;
}