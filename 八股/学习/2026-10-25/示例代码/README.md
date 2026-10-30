---
date: 2026-10-25
type: 示例代码索引
---

# 2026-10-25 示例代码索引

← [[学习/学习导航/2026-10-25-学习导航|今日导航]] · [[学习/2026-10-25/示例验证|编译验证记录]]

| 编号 | 阅读版 | 源文件 | 对应的题 | 观察重点 |
| --- | --- | --- | --- | --- |
| 03 | [[学习/2026-10-25/示例代码/03_cache_false_sharing\|03_cache_false_sharing.md]] | [[学习/2026-10-25/示例代码/03_cache_false_sharing.cpp\|.cpp]] | PERF-CACHE-001 / PERF-FALSESHARE-001 | AoS 与 SoA 的耗时差异；同一缓存行与分行放置的耗时差异 |
| 04 | [[学习/2026-10-25/示例代码/04_object_pool\|04_object_pool.md]] | [[学习/2026-10-25/示例代码/04_object_pool.cpp\|.cpp]] | CPP-POOL-001 / EX-20261025-001 | placement new 构造、显式析构、地址复用、池满行为 |

## 编译命令

```bash
# 单线程测量示例
g++ -std=c++17 -O2 -pthread 03_cache_false_sharing.cpp -o 03_cache_false_sharing

# 对象池示例
g++ -std=c++17 -O0 -Wall -Wextra 04_object_pool.cpp -o 04_object_pool
```

`03` 必须开优化才接近真实表现；`04` 用 `-O0` 是为了让构造/析构输出看得清楚，两种都能编译通过。
