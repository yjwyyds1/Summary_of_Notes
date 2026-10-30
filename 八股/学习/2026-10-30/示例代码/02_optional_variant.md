---
date: 2026-10-30
type: 代码阅读版
source: 02_optional_variant.cpp
---

# 02_optional_variant：两种“可能不是你以为的那样”

← [[学习/2026-10-30/04-optional与缺失值|optional]] · [[学习/2026-10-30/05-variant与事件类型|variant]] · [[学习/学习导航/2026-10-30-学习导航|今日导航]]

## 这份代码在做什么

1. 打印 `optional` / `variant` 的大小；
2. 用 `optional<double>` 表示“查不到价格”；
3. 用 `variant<Quote, Trade, Ack>` 表示三类事件，并用 `visit` 分发；
4. 演示空 `optional` 与错误类型 `get<T>` 的异常。

## 完整源码（与 `.cpp` 逐字一致）

```cpp
// ============================================================================
// 02_optional_variant.cpp
// 目的：看 optional / variant 怎么表达“可能没有值”与“几种之一”。
// 前置：知道 optional 的 has_value/value_or、variant 的 visit（04、05 号专题）。
// 程序步骤：
//   1. 打印 optional<T> 与 variant<Ts...> 的大小，理解内联存储；
//   2. 用 optional 表示“查不到的行情价”；
//   3. 用 variant 表示三类行情事件，并用 visit 分发；
//   4. 演示空 optional 的 value() 与错误类型 get<T>() 抛出的异常。
// 观察点：
//   - optional 没有堆分配，大小约等于 T 加一个标志位；
//   - variant 大小约等于最大成员加索引；
//   - visit 漏写分支会编译失败；get<T> 类型不符抛 bad_variant_access。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 02_optional_variant.cpp -o 02_optional_variant
// ============================================================================

#include <iostream>    // std::cout
#include <optional>    // std::optional
#include <string>      // std::string
#include <variant>     // std::variant、std::visit、std::get

// 三类事件：行情、成交、订单回报
struct QuoteEvent { double price = 0.0; int qty = 0; };
struct TradeEvent { double price = 0.0; int qty = 0; };
struct AckEvent   { long order_id = 0; bool accepted = false; };

using Event = std::variant<QuoteEvent, TradeEvent, AckEvent>;

// “重载集合”惯用法：把多个 lambda 合成一个可调用对象
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// 模拟“查行情价”：只有 1001 这个标的能查到
std::optional<double> lookup_price(int symbol_id) {
    if (symbol_id == 1001) {
        return 100.5;
    }
    return std::nullopt;   // 查不到
}

int main() {
    std::cout << "=== 1. 大小 ===" << "\n";
    std::cout << "sizeof(std::optional<char>)   = " << sizeof(std::optional<char>) << "\n";
    std::cout << "sizeof(std::optional<double>) = " << sizeof(std::optional<double>) << "\n";
    std::cout << "sizeof(Event)                 = " << sizeof(Event)
              << "（最大成员 AckEvent 的大小 + 索引 + 对齐）\n";

    std::cout << "\n=== 2. optional 表示可能没有值 ===" << "\n";
    if (auto price = lookup_price(1001)) {
        std::cout << "1001 的价格 = " << *price << "\n";
    }
    if (auto price = lookup_price(2002)) {
        std::cout << "不会走到这里\n";
    } else {
        std::cout << "2002 查不到，value_or 给默认值 = " << lookup_price(2002).value_or(-1.0) << "\n";
    }

    try {
        (void)lookup_price(2002).value();   // 空 optional 调用 value() 抛异常
    } catch (const std::bad_optional_access&) {
        std::cout << "空 optional 调用 value()：捕获到 std::bad_optional_access\n";
    }

    std::cout << "\n=== 3. variant 表示几种之一 ===" << "\n";
    Event events[] = {QuoteEvent{100.5, 200}, TradeEvent{100.6, 50}, AckEvent{9001, true}};

    for (const Event& event : events) {
        std::visit(overloaded{
            [](const QuoteEvent& e) { std::cout << "行情：price=" << e.price << "\n"; },
            [](const TradeEvent& e) { std::cout << "成交：qty=" << e.qty << "\n"; },
            [](const AckEvent& e)   { std::cout << "回报：order=" << e.order_id
                                                << (e.accepted ? " 已接受" : " 已拒绝") << "\n"; }
        }, event);
    }

    std::cout << "\n=== 4. 取错类型 ===" << "\n";
    Event ack = AckEvent{9002, false};
    std::cout << "holds_alternative<AckEvent> = "
              << (std::holds_alternative<AckEvent>(ack) ? "是" : "否") << "\n";
    try {
        (void)std::get<QuoteEvent>(ack);   // 当前不是 QuoteEvent
    } catch (const std::bad_variant_access&) {
        std::cout << "get<QuoteEvent> 取错：捕获到 std::bad_variant_access\n";
    }
    return 0;
}
```

## 本机运行结果（GCC 13.1，x86-64，`-O0`）

```text
=== 1. 大小 ===
sizeof(std::optional<char>)   = 2
sizeof(std::optional<double>) = 16
sizeof(Event)                 = 24（最大成员 AckEvent 的大小 + 索引 + 对齐）

=== 2. optional 表示可能没有值 ===
1001 的价格 = 100.5
2002 查不到，value_or 给默认值 = -1
空 optional 调用 value()：捕获到 std::bad_optional_access

=== 3. variant 表示几种之一 ===
行情：price=100.5
成交：qty=50
回报：order=9001 已接受

=== 4. 取错类型 ===
holds_alternative<AckEvent> = 是
get<QuoteEvent> 取错：捕获到 std::bad_variant_access
```

## 怎么读结果

- `optional<char>` 2 字节、`optional<double>` 16 字节：值 + 标志位 + 对齐，没有堆分配。
- `Event` 24 字节：最大成员 `AckEvent`（16 字节）加索引与对齐。
- `visit` 的分支如果漏写某个类型，编译会失败——这是 `variant` 相比“枚举 + 强转”的核心优势。

## 对应测试题

[[八股测试/2026-10-30#GEN-OPTIONAL-001]] · [[八股测试/2026-10-30#GEN-VARIANT-001]]
