---
date: 2026-10-30
type: 示例代码索引
---

# 2026-10-30 示例代码索引

← [[学习/学习导航/2026-10-30-学习导航|今日导航]] · [[学习/2026-10-30/示例验证|编译验证记录]]

| 编号 | 阅读版 | 源文件 | 对应的题 | 观察重点 |
| --- | --- | --- | --- | --- |
| 01 | [[学习/2026-10-30/示例代码/01_lambda_function\|01_lambda_function.md]] | [[学习/2026-10-30/示例代码/01_lambda_function.cpp\|.cpp]] | GEN-LAMBDA-001 / GEN-FUNCTION-001 | 闭包随捕获变大、`std::function` 大小固定；`shared_ptr` 捕获延长寿命 |
| 02 | [[学习/2026-10-30/示例代码/02_optional_variant\|02_optional_variant.md]] | [[学习/2026-10-30/示例代码/02_optional_variant.cpp\|.cpp]] | GEN-OPTIONAL-001 / GEN-VARIANT-001 | 无堆分配的大小；`visit` 分发；两类异常 |
| 03 | [[学习/2026-10-30/示例代码/03_forwarding\|03_forwarding.md]] | [[学习/2026-10-30/示例代码/03_forwarding.cpp\|.cpp]] | GEN-FORWARD-001 / EX-20261030-001 | 拷贝/移动计数；`forward` 保留值类别 |

## 编译命令

```bash
g++ -std=c++17 -O0 -Wall -Wextra 01_lambda_function.cpp -o 01_lambda_function
g++ -std=c++17 -O0 -Wall -Wextra 02_optional_variant.cpp -o 02_optional_variant
g++ -std=c++17 -O0 -Wall -Wextra 03_forwarding.cpp -o 03_forwarding
```
