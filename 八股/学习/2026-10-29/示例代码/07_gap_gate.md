# 练习：行情缺口时阻止新订单 · 注释源码阅读版

先修：[[学习/2026-10-29/07-练习-行情缺口与发单|专题详解]]。以下与保存的 cpp 和专题代码块逐字一致。

```cpp
// 目的：综合演示行情序号缺口与“暂停依赖该簿的新单”。
// 前置：仅为单品种、单更新号教学模型；真实 Binance 消息有 [U,u] 区间。
// 步骤：快照 200，收到 201、202、204，再用新快照 210 恢复。
#include <iostream> // 打印每一步是否允许新单。

class FeedGate {
    int last_seq_ = -1;     // 尚未有快照时记为 -1。
    bool trusted_ = false;  // 行情簿是否可供本题策略使用。
    bool manually_halted_ = false; // 独立的人工停发开关。
public:
    void load_snapshot(int seq) { last_seq_ = seq; trusted_ = true; }
    void on_sequence(int seq) {
        if (!trusted_ || seq <= last_seq_) return; // 不可信或旧事件不推进。
        if (seq != last_seq_ + 1) { trusted_ = false; return; }
        last_seq_ = seq; // 连续时才推进本地序号。
    }
    bool can_submit() const { return trusted_ && !manually_halted_; }
    int seq() const { return last_seq_; }
};

int main() {
    FeedGate gate; gate.load_snapshot(200);
    gate.on_sequence(201); gate.on_sequence(202);
    gate.on_sequence(204); // 203 缺失，不应把 last 改成 204。
    std::cout << gate.seq() << ',' << gate.can_submit() << std::endl; // 202,0。
    gate.load_snapshot(210); // 教学恢复；真实系统还需衔接缓存增量。
    std::cout << gate.seq() << ',' << gate.can_submit() << std::endl; // 210,1。
}
```

## 读后自检

若收到重复的 202，应推进序号吗？如果停发期间旧订单成交了，订单状态与风险预留要不要更新？

这份源码只证明教学模型中的步骤；生产规则和边界见专题详解。
