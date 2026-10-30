---
date: 2026-10-22
type: 代码阅读版
source: 07_filter_interface.cpp
---

# 07_filter_interface：练习题参考实现

← [[学习/2026-10-22/07-练习-过滤器接口约束|练习详解]] · [[学习/学习导航/2026-10-22-学习导航|今日导航]]

## 这份代码在做什么

把练习题里的错误版本改成“抽象基类 + 虚析构 + 禁拷贝 + const 引用”，并用两个实现（`MinPrice`、`Dedup`）验证：

- 基类引用调用不会切片，`Dedup` 的内部状态能保留；
- `unique_ptr<Filter>` 容器能正确销毁派生对象；
- `sizeof` 与“vptr + 成员”的推断一致。

## 完整源码（与 `.cpp` 逐字一致）

```cpp
// ============================================================================
// 07_filter_interface.cpp
// 目的：练习题的参考实现——给行情过滤器接口补上“虚析构、禁拷贝、const 引用”三个约束。
// 前置：学过虚函数、对象切片、拷贝控制默认行为。
// 程序步骤：
//   1. 定义不可拷贝、虚析构的抽象基类 Filter；
//   2. 写两个派生过滤器：MinPrice（最低价）与 Dedup（同价去重，演示有状态过滤器）；
//   3. 用基类引用调用，观察多态生效，且不发生切片；
//   4. 打印 sizeof，核对“vptr + 成员”的布局。
// 观察点：
//   - 删除拷贝构造/拷贝赋值后，任何按值传递的写法都会在编译期报错；
//   - 基类引用/指针不会复制对象，派生类状态得以保留；
//   - sizeof(MinPrice) = 8(vptr) + 8(double) = 16。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 07_filter_interface.cpp -o 07_filter_interface
// ============================================================================

#include <iostream>   // std::cout
#include <memory>     // std::unique_ptr
#include <vector>     // std::vector
#include <cmath>      // std::fabs

// 一条简化行情：只保留价格和数量
struct Quote {
    double price = 0.0;
    int qty = 0;
};

// ---------------------------------------------------------------------------
// 抽象基类：所有过滤器都必须满足的约束
// ---------------------------------------------------------------------------
class Filter {
public:
    virtual ~Filter() = default;   // 经基类指针 delete 时能销毁派生部分

    Filter() = default;
    Filter(const Filter&) = delete;             // 禁止拷贝构造：从根上杜绝切片
    Filter& operator=(const Filter&) = delete;  // 禁止拷贝赋值

    // 抽象操作：用 const 引用避免复制整条行情，const 表示不修改行情
    virtual void apply(const Quote& quote) const = 0;

    virtual const char* name() const = 0;
};

// ---------------------------------------------------------------------------
// 派生实现 1：价格低于阈值的行情丢弃
// ---------------------------------------------------------------------------
class MinPrice : public Filter {
public:
    explicit MinPrice(double min_price) : min_price_(min_price) {}
    void apply(const Quote& quote) const override {
        if (quote.price >= min_price_) {
            std::cout << "  [MinPrice] 通过 " << quote.price << "\n";
        }
    }
    const char* name() const override { return "MinPrice"; }
private:
    double min_price_ = 0.0;
};

// ---------------------------------------------------------------------------
// 派生实现 2：与上一条价格相同则丢弃（有状态，最能说明切片问题）
// ---------------------------------------------------------------------------
class Dedup : public Filter {
public:
    void apply(const Quote& quote) const override {
        // last_price_ 需要被修改，因此声明为 mutable：逻辑上不属于“对外可见状态”
        if (has_last_ && std::fabs(quote.price - last_price_) < 1e-9) {
            std::cout << "  [Dedup] 丢弃重复价 " << quote.price << "\n";
            return;
        }
        last_price_ = quote.price;
        has_last_ = true;
        std::cout << "  [Dedup] 保留 " << quote.price << "\n";
    }
    const char* name() const override { return "Dedup"; }
private:
    mutable double last_price_ = 0.0;   // mutable：const 成员函数里允许修改
    mutable bool has_last_ = false;
};

// 接收基类引用：不复制对象、不切片、派生类状态保留
static void feed(const Filter& filter, const std::vector<Quote>& quotes) {
    std::cout << "使用过滤器 " << filter.name() << "：\n";
    for (const Quote& quote : quotes) {
        filter.apply(quote);
    }
}

int main() {
    const std::vector<Quote> quotes = {
        {10.0, 100}, {10.0, 200}, {9.5, 50}, {10.5, 300}, {10.5, 400}
    };

    MinPrice min_price(10.0);
    Dedup dedup_filter;
    feed(min_price, quotes);
    feed(dedup_filter, quotes);

    std::cout << "\n=== sizeof 观察（x86-64 / GCC）===\n";
    std::cout << "sizeof(Filter)     = " << sizeof(Filter) << "   // 只有 vptr\n";
    std::cout << "sizeof(MinPrice)   = " << sizeof(MinPrice) << "  // vptr(8) + double(8)\n";
    std::cout << "sizeof(Dedup)      = " << sizeof(Dedup) << "  // vptr(8) + double(8) + bool(1) + 补齐\n";

    // 用智能指针管理多态对象：析构由虚函数保证正确销毁派生部分
    std::vector<std::unique_ptr<Filter>> filters;
    filters.push_back(std::make_unique<MinPrice>(9.0));
    filters.push_back(std::make_unique<Dedup>());
    std::cout << "\n=== 通过基类指针容器统一调用 ===\n";
    for (const auto& filter : filters) {
        filter->apply(Quote{9.8, 10});
    }
    return 0;
}
```

## 本机运行结果（GCC 13.1，x86-64，`-O0`）

```text
使用过滤器 MinPrice：
  [MinPrice] 通过 10
  [MinPrice] 通过 10
  [MinPrice] 通过 10.5
  [MinPrice] 通过 10.5
使用过滤器 Dedup：
  [Dedup] 保留 10
  [Dedup] 丢弃重复价 10
  [Dedup] 保留 9.5
  [Dedup] 保留 10.5
  [Dedup] 丢弃重复价 10.5

=== sizeof 观察（x86-64 / GCC）===
sizeof(Filter)     = 8   // 只有 vptr
sizeof(MinPrice)   = 16  // vptr(8) + double(8)
sizeof(Dedup)      = 24  // vptr(8) + double(8) + bool(1) + 补齐

=== 通过基类指针容器统一调用 ===
  [MinPrice] 通过 9.8
  [Dedup] 保留 9.8
```

## 怎么读结果

- `Dedup` 第二次遇到 10.0 时丢弃、遇到 10.5 再丢弃一次，说明状态在多次调用之间保留下来了；如果当初按值传 `Filter`，这份状态会在切片时丢失。
- `sizeof(Dedup)` 是 24：vptr 8 + double 8 + bool 1，再补到 8 的倍数。

## 对应测试题

[[八股测试/2026-10-22#EX-20261022-001 · 过滤器接口练习]]
