---
date: 2026-10-21
standard: C++17
status: 未自测
---

# 03_override · 完整中文注释源码

[[学习/学习导航/2026-10-21-学习导航|当天学习入口]] · [[学习/2026-10-21/03-override与名字隐藏|对应学习正文]]

[[八股测试/2026-10-21#CPP-OVERRIDE-001 · 同名函数为什么没有覆盖成功|对应自测]]

以下围栏中的内容与同名 .cpp 完整逐字一致，可复制为单独的 C++17 程序。

本机 GCC 13.1 的当前 `-Wall` 构建预期报告 `-Woverloaded-virtual`：`Almost::score()` 合法隐藏了基类的 `score() const`。
这是故意保留的教学警告，正帮助发现少写 `const`；后面的 `Fixed` 展示正确覆盖，程序仍可正常编译运行。
warning（警告）在本次选项下允许继续构建；给错误签名加 `override` 则会产生 error（编译错误）。
不同编译器、警告开关或“警告视为错误”选项可能改变诊断形式及构建结果，不能把本机表现当作标准要求。
先读 `Almost` 与 `Fixed` 的差别，再运行各组断言；全部通过后标准输出为 `override, hiding, using and final checks passed`。

```cpp
// C++17：覆盖、重载、名字隐藏以及 using / final 的不同职责。
// 阅读步骤：先比较 Almost 的少 const，再看 Fixed 的正确覆盖，最后看 using / final。
// 编译观察：本机 GCC 13.1 在当前 -Wall 配置下预期给出 -Woverloaded-virtual 警告。
// 这是 Almost::score() 合法隐藏 Base::score() const 的教学例子，不抑制该诊断。
// warning 帮助发现可疑代码，当前构建仍可生成程序；注释里的 override 错配才是 error。
// 运行步骤：依次执行各组 assert；条件全部成立后输出下面一行并正常退出。
// 预期标准输出：override, hiding, using and final checks passed
#include <cassert>  // assert 用具体返回值核对究竟选中了哪个函数。
#include <iostream> // std::cout 输出最终检查结果。

class Base {
public:
    // score 的函数末尾有 const，它也是覆盖匹配条件的一部分。
    virtual int score() const { return 10; }

    // 两个 report 同名、参数不同，在 Base 内形成重载。
    int report(int) const { return 100; }
    int report(double) const { return 200; }

    virtual ~Base() = default;
};

class Almost : public Base {
public:
    // 故意少写 const：它隐藏 score 名字，但没有覆盖 Base::score。
    // 本机编译器的隐藏警告正帮助发现这个差别；这里保留它，供比较两种调用结果。
    // 这段是合法 C++；后面的 Fixed 用 const override 展示应采用的修正形式。
    int score() { return 99; }

    // 只留纸面编译错误：int score() override { return 99; }
    // 加上 override 会因无法覆盖 Base::score() const 而拒绝编译。
};

class Hidden : public Base {
public:
    // 名字 report 在当前类找到了，普通查找不会再合并基类重载。
    int report(int) const { return 300; }
};

class Fixed : public Base {
public:
    // 名字、参数、const 匹配，返回类型也合法，确实覆盖。
    int score() const override { return 20; }

    // 将基类的 report 声明带入本类的重载候选集合。
    using Base::report;
    int report(int) const { return 300; }
};

// 类名后写 final：禁止继续从 Frozen 派生。
class Frozen final : public Fixed {
public:
    int score() const override { return 30; }
};

class Locked : public Base {
public:
    // 函数后写 final：禁止下层再次覆盖这个虚函数。
    int score() const override final { return 40; }
};

// Locked 自己仍可作为基类；禁止覆盖某个函数不等于禁止派生。
class ChildOfLocked : public Locked {};

// 编译错误示意，只作注释：class BadClass : public Frozen {};
// 编译错误示意：class BadMethod : public Locked {
//     int score() const override { return 50; }
// };

int main() {
    Almost almost;
    const Base& almost_base = almost;
    assert(almost.score() == 99);
    assert(almost_base.score() == 10);
    // const Almost& ca = almost; ca.score(); // 被非 const 同名函数隐藏。

    Hidden hidden;
    // 只有 Hidden::report(int) 成为候选，3.5 隐式转换为 int。
    assert(hidden.report(3.5) == 300);
    assert(hidden.Base::report(3.5) == 200);

    Fixed fixed;
    const Base& fixed_base = fixed;
    assert(fixed.score() == 20);
    assert(fixed_base.score() == 20);
    assert(fixed.report(3) == 300);
    assert(fixed.report(3.5) == 200);

    Frozen frozen;
    const Base& frozen_base = frozen;
    assert(frozen_base.score() == 30);

    ChildOfLocked child;
    const Base& locked_base = child;
    assert(locked_base.score() == 40);
    std::cout << "override, hiding, using and final checks passed\n";
}
```
