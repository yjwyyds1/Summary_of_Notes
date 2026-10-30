---
date: 2026-10-28
type: 示例代码索引
---

# 2026-10-28 示例代码索引

← [[学习/学习导航/2026-10-28-学习导航|今日导航]] · [[学习/2026-10-28/示例验证|编译验证记录]]

| 编号 | 阅读版 | 源文件 | 对应的题 | 观察重点 |
| --- | --- | --- | --- | --- |
| 01 | [[学习/2026-10-28/示例代码/01_link_error\|01_link_error.md]] | [01_link_error.h](01_link_error.h) · [01_link_error_main.cpp](01_link_error_main.cpp) · [01_link_error_util.cpp](01_link_error_util.cpp) | BUILD-ODR-001 / EX-20261028-001 | 只有声明 → 编译通过、链接失败；`nm` 中 `U` 与 `T` 的区别 |
| 02 | [[学习/2026-10-28/示例代码/02_perf_benchmark\|02_perf_benchmark.md]] | [[学习/2026-10-28/示例代码/02_perf_benchmark.cpp\|.cpp]] | PERF-PROFILE-001 | 预热、多轮取最小、`volatile` 防优化；reserve 前后的容量与耗时 |

## 复现命令

```bash
# 1. 复现链接错误（预期失败）
g++ -std=c++17 -O0 01_link_error_main.cpp -o link_demo

# 2. 修复后编译运行
g++ -std=c++17 -O0 -Wall -Wextra 01_link_error_main.cpp 01_link_error_util.cpp -o link_demo
./link_demo

# 3. 微基准
g++ -std=c++17 -O2 -Wall -Wextra 02_perf_benchmark.cpp -o 02_perf_benchmark
./02_perf_benchmark
```

第 1 条命令失败是**预期结果**，它正是当天练习要你诊断的现象。

单独编译 `01_link_error_util.cpp` 也会失败，原因是它没有 `main`（报错形如 `undefined reference to 'main'`），这属于正常现象：它是给主程序提供定义的翻译单元，必须与 `01_link_error_main.cpp` 一起链接。
