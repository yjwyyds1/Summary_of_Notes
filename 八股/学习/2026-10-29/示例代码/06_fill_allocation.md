# 批量成交分配：先定义公平再写算法 · 注释源码阅读版

先修：[[学习/2026-10-29/06-批量成交分配|专题详解]]。以下与保存的 cpp 和专题代码块逐字一致。

```cpp
// 目的：按比例和最大余数法分配一次外部成交，演示整数守恒。
// 前置：这里假设成交量不超过总需求，公平政策已约定为按需求比例。
// 步骤：需求 5、3、2，成交 5；先取下界再按余数分配。
#include <algorithm> // std::sort 按余数排序候选客户。
#include <iostream>  // 打印每个客户的最终手数。
#include <numeric>   // std::accumulate 计算总需求与已分配数量。
#include <stdexcept> // std::invalid_argument 表示本题输入不符合前提。
#include <vector>    // 保存客户需求、余数和结果。

std::vector<int> allocate(const std::vector<int>& want, int fill) {
    if (want.empty() || fill < 0) throw std::invalid_argument("bad fill");
    for (int q : want) if (q < 0) throw std::invalid_argument("bad want");
    const long long total = std::accumulate(want.begin(), want.end(), 0LL);
    if (fill > total || (total == 0 && fill != 0))
        throw std::invalid_argument("overfill");
    std::vector<int> out(want.size(), 0);
    if (total == 0) return out; // 零需求、零成交，不做除法。
    std::vector<long long> remainder(want.size(), 0);
    std::vector<int> order(want.size());
    std::iota(order.begin(), order.end(), 0); // 客户原始顺序作并列规则。
    for (std::size_t i = 0; i < want.size(); ++i) {
        const long long scaled = 1LL * fill * want[i]; // 教学输入很小。
        out[i] = static_cast<int>(scaled / total); // 比例份额的整数下界。
        remainder[i] = scaled % total;             // 未分到的小数部分。
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (remainder[a] != remainder[b]) return remainder[a] > remainder[b];
        return a < b; // 余数相同，原顺序靠前者先拿一手。
    });
    int left = fill - std::accumulate(out.begin(), out.end(), 0);
    for (int i : order) {
        if (left == 0) break;
        if (out[i] < want[i]) { ++out[i]; --left; }
    }
    if (left != 0) throw std::logic_error("allocation invariant");
    return out;
}

int main() {
    for (int n : allocate({5, 3, 2}, 5)) std::cout << n << ' ';
    std::cout << std::endl; // 3 1 1；不是唯一的业务公平政策。
}
```

## 读后自检

若需求 `[1,1,1]`、成交 2，按本题并列规则谁拿到？为何不能分别四舍五入 2/3？

这份源码只证明教学模型中的步骤；生产规则和边界见专题详解。
