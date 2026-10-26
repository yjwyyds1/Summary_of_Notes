//
// Created by 20675 on 2026/10/19.
//
// ============================================================
//  专题实验: nullptr 的 0 + const 锁谁
//  编译: g++ -std=c++17 -g const_null.cpp -o const_null
// ============================================================

#include <iostream>

int main()
{
    // ================= 第一部分: nullptr 打印的 0 =================

    std::cout << "===== 第一部分: nullptr 的真面目 =====\n";

    int x { 5 };
    int* pX    { &x };     // 纸条上写着 x 的真门牌号
    int* pNull { nullptr };// 纸条上写着"无"

    std::cout << "pX(有号的纸条)打印:    " << pX << '\n';    // 一长串地址, 如 0x7ffee3c5a2ac
    std::cout << "pNull(写着无)打印:     " << pNull << '\n'; // 0 (或 0x0 / 00000000)
    std::cout << "x 自己的地址(&x):      " << &x << '\n';    // 和 pX 打印的一样!

    std::cout << "\n--- 关键: 判断空指针靠比较, 不靠肉眼看 0 ---\n";
    std::cout << "pNull == nullptr ?  " << (pNull == nullptr) << "  (1 = 是空)\n";
    std::cout << "pX    == nullptr ?  " << (pX == nullptr)    << "  (0 = 不是空)\n";

    std::cout << "\n--- if(p) 是最常用的判空写法 ---\n";
    if (pNull)
        std::cout << "pNull 有效, *pNull = " << *pNull << '\n';
    else
        std::cout << "pNull 是空的, 不能 *pNull (闯 0 号禁房会崩溃!)\n";

    if (pX)
        std::cout << "pX 有效, *pX = " << *pX << '\n';

    std::cout << "\n--- 实验: nullptr 字面量直接丢给 cout (各家输出不同, 看看你的) ---\n";
    std::cout << "直接打印 nullptr: " << nullptr << '\n';

    // 千万别这么干 (注释掉了, 因为真执行程序会当场崩溃):
    // *pNull = 1;    // 解引用空指针 = 硬闯 0 号禁房 = 未定义行为/崩溃


    // ================= 第二部分: const 到底锁谁 =================

    std::cout << "\n===== 第二部分: const 锁谁 =====\n";

    int a { 1 };
    int b { 2 };

    // ---- 形态 1: const int* p  (const 左边是 int → 锁值) ----
    const int* p1 { &a };
    std::cout << "[形态1] const int* p1  -> 锁值, 纸条自由\n";
    // *p1 = 10;   // ① 试试取消注释 → 报错! 值被锁, 不许通过 p1 改
    p1 = &b;       // ✅ 纸条可以重写, 现在 p1 指向 b
    std::cout << "  p1 重指向 b 成功, *p1 = " << *p1 << '\n';
    a = 99;        // ✅ a 本人不是 const, 直接改 a 没问题
    std::cout << "  a 本人仍可直接改, a = " << a << "\n\n";

    // ---- 形态 2: int* const p  (const 左边是 * → 锁指针) ----
    int* const p2 { &a };
    std::cout << "[形态2] int* const p2  -> 锁指针, 值自由\n";
    // p2 = &b;    // ② 试试取消注释 → 报错! 纸条被锁, 不许重写
    *p2 = 111;     // ✅ 值可以改
    std::cout << "  *p2=111 成功, a = " << a << '\n';
    std::cout << "  但 p2 永远指向 a, p2==&a ? " << (p2 == &a) << "\n\n";

    // ---- 形态 3: const int* const p  (双锁) ----
    const int* const p3 { &a };
    std::cout << "[形态3] const int* const p3 -> 双锁, 只能读\n";
    // *p3 = 1;    // ③ 取消注释 → 报错! 值锁
    // p3 = &b;    // ④ 取消注释 → 报错! 指针锁
    std::cout << "  只能读: *p3 = " << *p3 << "\n\n";

    // ---- 加餐: int const* 就是 const int* (换写法不换意思) ----
    int const* p4 { &a };       // 等价于 const int* p4
    std::cout << "[加餐] int const* p4 -> 也是锁值 (和形态1同一个意思)\n";
    // *p4 = 1;    // ⑤ 取消注释 → 报错! 照样锁值
    p4 = &b;       // ✅ 纸条自由
    std::cout << "  p4 重指向 b 成功, *p4 = " << *p4 << "\n\n";

    // ---- 加餐: const 引用 = 只读外号 ----
    const int& r { a };         // 锁值; 引用天生锁指向 → 双保险只读
    std::cout << "[加餐] const int& r -> 只读外号, r = " << r << '\n';
    // r = 5;      // ⑥ 取消注释 → 报错! 不许通过外号改
    a = 42;        // ✅ a 本人仍可改
    std::cout << "  改 a 本人后, r 看到的是 " << r << " (外号跟着本体)\n";

    std::cout << "\n===== 实验结束 =====\n";
    return 0;
}