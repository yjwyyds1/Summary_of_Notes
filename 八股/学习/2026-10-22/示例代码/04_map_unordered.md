---
date: 2026-10-22
cpp_standard: C++17
type: 代码阅读版
source: 04_map_unordered.cpp
---

# 04_map_unordered · 有序键与哈希查找

← [[学习/学习导航/2026-10-22-学习导航|学习导航]] · [[学习/2026-10-22/04-map与unordered_map|逐步讲解]] · [[学习/2026-10-22/示例代码/04_map_unordered.cpp|可编译源码]]

两个容器保存相同的三组“键→值”。阅读时找出哪些结果有顺序保证，哪些不能预测。完整代码与同名 cpp 一致。

```cpp
// C++17 学习示例：对照 map 的有序键与 unordered_map 的单点查找。
// 这里只观察标准可保证的顺序与查找，不猜 unordered_map 的遍历顺序。

#include <cassert>       // assert：验证顺序与找到的值。
#include <iostream>      // std::cout：显示可观察结果。
#include <map>           // std::map：按比较器排列唯一键。
#include <string>        // std::string：使用文字作为键。
#include <unordered_map> // std::unordered_map：以哈希和相等关系查找键。

int main() {
    // 两个容器都保存“键 -> 值”；这里只需查找，不需要自定义键类型。
    std::map<std::string, int> ordered{{"ccc", 3}, {"aaa", 1}, {"bbb", 2}};
    std::unordered_map<std::string, int> hashed{
        {"ccc", 3}, {"aaa", 1}, {"bbb", 2}};

    // 默认比较器按字符串顺序排键，因此 map 的首项是 aaa。
    assert(ordered.begin()->first == "aaa");
    // find 返回迭代器；没找到时返回 end，先比较再访问 second。
    const auto ordered_hit = ordered.find("bbb");
    const auto hash_hit = hashed.find("bbb");
    assert(ordered_hit != ordered.end() && ordered_hit->second == 2);
    assert(hash_hit != hashed.end() && hash_hit->second == 2);
    assert(hashed.find("missing") == hashed.end());

    std::cout << "map first key: " << ordered.begin()->first << '\n';
    std::cout << "both find bbb: " << ordered_hit->second << ' '
              << hash_hit->second << '\n';
    // 不打印或断言 hashed.begin() 是某个固定键：其遍历顺序未规定。
}
```

**运行观察：** map 的首键是 `aaa`，两个容器都能找到 `bbb` 的值 `2`。不写任何针对 unordered_map 首键的断言。

> [!question]- 自测
> 1. `find` 找不到时返回什么？
> 2. 为什么要先检查迭代器是否等于 end？
> 3. 哪一个容器提供按键顺序遍历？
> 4. 为何不能把 unordered_map 的一次输出顺序当保证？

> [!success]- 核对
> 返回 end；直接解引用 end 是错误用法。map 按比较器的键序排列，无序容器不承诺固定遍历顺序。
