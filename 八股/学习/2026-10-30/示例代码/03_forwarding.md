---
date: 2026-10-30
type: 代码阅读版
source: 03_forwarding.cpp
---

# 03_forwarding：用拷贝/移动计数看懂 forward

← [[学习/2026-10-30/03-完美转发与引用折叠|完美转发]] · [[学习/2026-10-30/07-练习-消息工厂|练习]] · [[学习/学习导航/2026-10-30-学习导航|今日导航]]

## 这份代码在做什么

1. 定义一个会打印“拷贝/移动”的 `Tagged` 类型；
2. 写 `make_event_bad`（不用 `forward`）与 `make_event_good`（用 `forward`）；
3. 分别用左值和右值调用，观察行为差异。

## 完整源码（与 `.cpp` 逐字一致）

```cpp
// ============================================================================
// 03_forwarding.cpp
// 目的：用“拷贝/移动计数”把完美转发讲成可观察的事实。
// 前置：知道左值/右值与 std::move（10-20），读过当天 03 号专题。
// 程序步骤：
//   1. 定义一个会打印“拷贝/移动”的类型 Tagged；
//   2. 写 make_event_bad（不用 forward）与 make_event_good（用 forward）两个工厂；
//   3. 分别用左值和右值调用，观察拷贝/移动次数；
//   4. 演示 std::string 在事件结构里的移动效果。
// 观察点：
//   - 不用 forward 时，参数有名字就是左值，永远走拷贝；
//   - 用 forward 时，传右值走移动、传左值走拷贝；
//   - 移动之后源对象处于“有效但未指定”状态，不要读它的值。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 03_forwarding.cpp -o 03_forwarding
// ============================================================================

#include <iostream>   // std::cout
#include <string>     // std::string
#include <utility>    // std::move、std::forward

// ---------------------------------------------------------------------------
// 会打印自身拷贝/移动的观测类型
// ---------------------------------------------------------------------------
class Tagged {
public:
    explicit Tagged(std::string text) : text_(std::move(text)) {
        std::cout << "    构造 Tagged\n";
    }
    Tagged(const Tagged& other) : text_(other.text_) {
        std::cout << "    拷贝 Tagged\n";
    }
    Tagged(Tagged&& other) noexcept : text_(std::move(other.text_)) {
        std::cout << "    移动 Tagged\n";
    }
    Tagged& operator=(const Tagged&) = default;
    Tagged& operator=(Tagged&&) noexcept = default;
    ~Tagged() = default;

    const std::string& text() const { return text_; }

private:
    std::string text_;
};

// 一个“事件”结构：内部持有 Tagged
struct Event {
    Tagged payload;
};

// ---------------------------------------------------------------------------
// 反例：没有 forward
// ---------------------------------------------------------------------------
template <typename EventT, typename... Args>
EventT make_event_bad(Args&&... args) {
    // args 是有名字的形参：无论调用处传左值还是右值，这里都是左值
    return EventT{args...};
}

// ---------------------------------------------------------------------------
// 正例：用 forward 保留值类别
// ---------------------------------------------------------------------------
template <typename EventT, typename... Args>
EventT make_event_good(Args&&... args) {
    // std::forward<Args> 按推导出的类型转发：右值仍是右值
    return EventT{std::forward<Args>(args)...};
}

int main() {
    std::cout << "=== 1. 不用 forward：右值也退化成拷贝 ===\n";
    Tagged source("order-1");
    std::cout << "  make_event_bad(std::move(source))\n";
    Event bad = make_event_bad<Event>(std::move(source));
    std::cout << "  结果文本：" << bad.payload.text() << "\n";

    std::cout << "\n=== 2. 用 forward：右值保持右值 ===\n";
    Tagged source2("order-2");
    std::cout << "  make_event_good(std::move(source2))\n";
    Event good = make_event_good<Event>(std::move(source2));
    std::cout << "  结果文本：" << good.payload.text() << "\n";

    std::cout << "\n=== 3. 用 forward 传左值：仍然是拷贝 ===\n";
    Tagged source3("order-3");
    std::cout << "  make_event_good(source3)\n";
    Event copied = make_event_good<Event>(source3);
    std::cout << "  结果文本：" << copied.payload.text() << "，源对象仍可用：" << source3.text() << "\n";

    std::cout << "\n=== 4. 直接移动构造，对比成本 ===\n";
    Tagged source4("order-4");
    std::cout << "  Event direct{std::move(source4)}\n";
    Event direct{std::move(source4)};
    std::cout << "  结果文本：" << direct.payload.text() << "\n";

    std::cout << "\n结论：forward 不创造移动，它只是不丢失调用处已经表达的意图\n";
    return 0;
}
```

## 本机运行结果（GCC 13.1，x86-64，`-O0`）

```text
=== 1. 不用 forward：右值也退化成拷贝 ===
    构造 Tagged
  make_event_bad(std::move(source))
    拷贝 Tagged
  结果文本：order-1

=== 2. 用 forward：右值保持右值 ===
    构造 Tagged
  make_event_good(std::move(source2))
    移动 Tagged
  结果文本：order-2

=== 3. 用 forward 传左值：仍然是拷贝 ===
    构造 Tagged
  make_event_good(source3)
    拷贝 Tagged
  结果文本：order-3，源对象仍可用：order-3

=== 4. 直接移动构造，对比成本 ===
    构造 Tagged
  Event direct{std::move(source4)}
    移动 Tagged
  结果文本：order-4
```

## 怎么读结果

- 第 1 段：调用处写了 `std::move`，但形参 `args` 有名字、是左值，最终仍然拷贝。
- 第 2 段：`std::forward<Args>` 把右值转发出去，触发移动。
- 第 3 段：传左值时 `forward` 不改变值类别，仍然是拷贝——这是正确行为，不是 bug。
- 移动之后源对象不要读；本例只在第 3 段读取了未被移动的 `source3`。

## 对应测试题

[[八股测试/2026-10-30#GEN-FORWARD-001]] · [[八股测试/2026-10-30#EX-20261030-001]]
