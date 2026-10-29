# 快照加增量：怎样恢复行情订单簿 · 注释源码阅读版

先修：[[学习/2026-10-29/03-快照增量与缺口恢复|专题详解]]。以下与保存的 cpp 和专题代码块逐字一致。

```cpp
// 目的：用单价位教学模型演示快照基线、旧更新、连续更新与缺口。
// 前置：本例借用 Binance Spot 的 U/u 区间语义，不泛化到所有行情。
// 步骤：快照 100，应用 101 和 102，遇到缺失 103 时进入不可信状态。
#include <iostream> // 打印序号、数量和可信标记。

struct Delta { int first; int last; int price; int absolute_qty; };

class LocalLevel {
    int last_seq_ = -1;   // -1 表示尚未加载快照。
    int quantity_ = 0;    // 教学上只维护价格 100 这一层。
    bool trusted_ = false; // gap 后必须重新同步，不能继续发出可信信号。
public:
    void load_snapshot(int seq, int qty) {
        last_seq_ = seq; quantity_ = qty; trusted_ = true;
    }
    bool apply(const Delta& d) {
        if (!trusted_ || d.price != 100 || d.first > d.last || d.absolute_qty < 0)
            return false; // 校验失败或状态不可信，不修改本地簿。
        if (d.last <= last_seq_) return true; // 事件完全旧，忽略即可。
        if (d.first > last_seq_ + 1) {
            trusted_ = false; return false; // 缺少中间更新，必须重新同步。
        }
        quantity_ = d.absolute_qty; // 协议给的是新绝对量，0 即删除该层。
        last_seq_ = d.last;          // 成功应用后才推进本地序号。
        return true;
    }
    int seq() const { return last_seq_; }
    int qty() const { return quantity_; }
    bool trusted() const { return trusted_; }
};

int main() {
    LocalLevel level; level.load_snapshot(100, 5);
    level.apply({99, 100, 100, 9});  // 旧更新不能覆盖快照。
    level.apply({101, 101, 100, 3}); // 绝对数量变为 3。
    level.apply({102, 102, 100, 0}); // 数量 0 即该层消失。
    level.apply({104, 104, 100, 7}); // 103 缺失，停止信任。
    std::cout << level.seq() << ',' << level.qty() << ','
              << level.trusted() << std::endl; // 102,0,0。
}
```

## 读后自检

快照 ID 100，收到 [99,100]、[101,101]、[103,103]，哪一条触发 gap？若消息数量为 0，价层如何处理？

这份源码只证明教学模型中的步骤；生产规则和边界见专题详解。
