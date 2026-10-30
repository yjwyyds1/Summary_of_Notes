---
date: 2026-10-25
topic: new/delete 与 malloc/free
question_id: CPP-NEWMALLOC-001
cpp_standard: C++17
---

# new/delete 和 malloc/free 到底差在哪

← [[学习/学习导航/2026-10-25-学习导航|今日导航]] · [[八股测试/2026-10-25#CPP-NEWMALLOC-001|回到测试题]]

## 先把词说清楚

**`malloc` / `free`**：C 标准库的堆分配函数，按**字节数**申请、按**指针**释放，不管对象是什么类型。

**`new` / `delete`**：C++ 运算符，按**类型**申请、调构造函数；`delete` 先调析构再释放内存。

**`operator new`**：`new` 表达式背后真正负责“要一块内存”的函数，可以被重载或替换。

三者的关系是：`new T(args)` ≈ `operator new(sizeof(T))` + 在返回的地址上构造 `T`。`delete p` ≈ 先 `p->~T()`，再 `operator delete(p)`。

## 它解决什么问题

`malloc` 只给一块原始内存，C++ 的对象还需要“初始化”这一步：成员要构造、虚表指针要设置、基类子对象要建立。如果只 `malloc` 再用，对象处于未初始化状态，成员对象的构造函数根本没跑。

`new` 把“分配 + 构造”合成一个原子操作，并且构造抛异常时自动释放内存，避免泄漏。

## 逐项对比

| 维度 | `malloc/free` | `new/delete` |
| --- | --- | --- |
| 所属 | C 标准库函数 | C++ 运算符 |
| 参数 | 字节数 | 类型（大小由编译器算） |
| 返回值 | `void*`，失败返回 `NULL` | `T*`，失败抛 `std::bad_alloc`（`nothrow` 版本返回空） |
| 构造/析构 | 不调用 | 调用 |
| 类型安全 | 无 | 有 |
| 对齐 | 满足 `max_align_t` | 满足类型对齐，C++17 支持过对齐 |
| 能否重载 | 不能（可用 hooks） | 可以全局或按类重载 |
| 能否扩容 | `realloc` | 不能，需要新分配 + 移动 |
| 配对 | `malloc` ↔ `free` | `new` ↔ `delete`，`new[]` ↔ `delete[]` |

## 最小代码

```cpp
// malloc：只有内存，没有对象
void* raw = std::malloc(sizeof(Widget));
Widget* bad = static_cast<Widget*>(raw);   // 未定义行为：Widget 从未被构造

// new：分配 + 构造
Widget* good = new Widget(42);             // 构造函数被调用
delete good;                               // 析构函数被调用，然后释放内存
```

数组要配对：

```cpp
int* array = new int[10];   // 10 个 int，默认初始化
delete[] array;             // 必须用 delete[]
```

`new[]` 与 `delete` 混用是未定义行为；对含析构函数的类型，多数实现需要靠“数组长度”信息逐个析构，用错运算符就找不到这份信息。

## 底层发生了什么

```mermaid
flowchart LR
  A["new Widget(42)"] --> B["operator new(sizeof(Widget))"]
  B --> C["堆分配器：brk/mmap 或空闲链表"]
  C --> D["返回原始内存"]
  D --> E["placement new：在地址上构造 Widget"]
  E --> F["返回 Widget*"]
  F --> G["delete p"]
  G --> H["p->~Widget()"]
  H --> I["operator delete(p)"]
```

`malloc` 的常见实现（glibc）会在每块内存前放一个 chunk 头，记录大小与状态；`free` 靠这个头知道要回收多少。这也解释了为什么 `free` 不需要你传长度，而 `delete[]` 需要单独的数组形式。

## 重载与 placement new

```cpp
class Session {
public:
    static void* operator new(std::size_t size);   // 按类重载：可以走内存池
    static void operator delete(void* pointer) noexcept;
};
```

```cpp
alignas(Session) unsigned char buffer[sizeof(Session)];
Session* placed = new (buffer) Session();   // placement new：只在已有内存上构造
placed->~Session();                          // 手动析构，内存不动
```

`placement new` 是对象池、共享内存、嵌入式固定缓冲区的核心手段：内存由别处提供，`new` 只负责构造。

## 边界与常见误解

**“new 一定比 malloc 慢？”** `new` 多了一次构造函数调用，但分配器可能是同一个；对 POD 类型，差距常常很小。真正影响延迟的是分配器的锁与碎片，不是这两个名字。

**“malloc 失败返回空，所以 new 也返回空？”** 默认 `new` 抛 `std::bad_alloc`；要用返回空的形式写 `new (std::nothrow) T`。

**“delete 之后指针变空了吗？”** 没有。`delete p` 不会把 `p` 置空，重复 `delete` 是未定义行为；建议 `delete p; p = nullptr;` 或用智能指针。

**“new 出来的对象一定在堆上？”** 重载 `operator new` 后不一定；placement new 更是在你给的地方。

**“free 一个 new 出来的指针？”** 未定义行为。反过来也一样。

## 使用场景

- **业务代码**：用 `new`/`make_unique`，让构造与析构成对出现。
- **底层/容器**：`operator new` 拿原始内存，再用 placement new 构造元素——`std::vector` 的扩容就是这么做的。
- **低延迟**：启动阶段预分配（对象池、环形缓冲），热路径避免任何动态分配。

## 变式与口述总结

> malloc/free 只管字节，不管对象；new/delete 是运算符，先分配再构造、先析构再释放，失败抛 bad_alloc，类型安全，可以按类重载。底层 `new T` 等价于 `operator new` 加 placement new。混用、`new[]`/`delete` 不配对都是未定义行为。

**变式：**

1. `new int[10]` 之后用 `delete` 而不是 `delete[]`，可能出什么问题？
2. 为什么 `free` 不需要长度，而 `delete[]` 需要单独的语法？
3. 对象池为什么要用 placement new？

## 来源

- 题型来源：[美团 C++ 后端面经](https://www.iamshuaidi.com/4313.html) 一面原问“malloc 与 new 的区别，free 与 delete 的区别”。镜像页未标发布日期，本次访问 2026-10-30。
- 标准依据：[N4659 expr.new](https://timsong-cpp.github.io/cppwp/n4659/expr.new)、[expr.delete](https://timsong-cpp.github.io/cppwp/n4659/expr.delete)、[basic.stc.dynamic.allocation](https://timsong-cpp.github.io/cppwp/n4659/basic.stc.dynamic.allocation)。
