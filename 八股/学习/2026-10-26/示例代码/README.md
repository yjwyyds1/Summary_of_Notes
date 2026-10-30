---
date: 2026-10-26
type: 示例代码索引
---

# 2026-10-26 示例代码索引

← [[学习/学习导航/2026-10-26-学习导航|今日导航]] · [[学习/2026-10-26/示例验证|编译验证记录]]

| 编号 | 阅读版 | 源文件 | 对应的题 | 观察重点 |
| --- | --- | --- | --- | --- |
| 01 | [[学习/2026-10-26/示例代码/01_topk\|01_topk.md]] | [[学习/2026-10-26/示例代码/01_topk.cpp\|.cpp]] | STL-HEAP-001 | 全排序 / 小根堆 / `nth_element` 的结果一致性与耗时差异 |
| 02 | [[学习/2026-10-26/示例代码/02_lru\|02_lru.md]] | [[学习/2026-10-26/示例代码/02_lru.cpp\|.cpp]] | EX-20261026-001 | `list::splice` 的 O(1) 前移；淘汰时两份结构同时维护 |
| 03 | [[学习/2026-10-26/示例代码/03_sort_stability\|03_sort_stability.md]] | [[学习/2026-10-26/示例代码/03_sort_stability.cpp\|.cpp]] | STL-SORT-001 | 小数组“看起来稳定”的假象；200 条时 `std::sort` 与 `stable_sort` 的差异 |

## 编译命令

```bash
g++ -std=c++17 -O2 01_topk.cpp -o 01_topk
g++ -std=c++17 -O0 -Wall -Wextra 02_lru.cpp -o 02_lru
g++ -std=c++17 -O0 -Wall -Wextra 03_sort_stability.cpp -o 03_sort_stability
```
