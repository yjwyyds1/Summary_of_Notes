# 订单回报状态与撤单竞态 · 注释源码阅读版

先修：[[学习/2026-10-29/05-订单回报与撤单竞态|专题详解]]。以下与保存的 cpp 和专题代码块逐字一致。

```cpp
// 目的：展示待撤期间继续成交、执行 ID 去重与最终取消剩余量。
// 前置：这只是 FIX 状态机的教学子集，不覆盖全部事件和字段。
// 步骤：5 手单先成 2，待撤又成 1，最后撤掉余下 2。
#include <iostream>      // 打印最终累计成交与剩余量。
#include <unordered_set> // 保存已处理的执行 ID，避免回报重放重复计数。

class OrderState {
    int original_ = 5; // 原始委托数量。
    int cum_ = 0;      // 累计已经成交的数量。
    int leaves_ = 5;   // 当前还能继续成交的数量。
    bool pending_cancel_ = false; // 已请求撤单，但未确认撤成。
    bool canceled_ = false;       // 收到明确取消确认后才为 true。
    std::unordered_set<int> exec_ids_;
public:
    bool on_fill(int exec_id, int qty) {
        if (canceled_ || qty <= 0 || qty > leaves_ || exec_ids_.count(exec_id))
            return false; // 重复、越量或终态后的异常输入都拒绝。
        exec_ids_.insert(exec_id);
        cum_ += qty; leaves_ -= qty; // 成交同时改两个数量字段。
        return true;
    }
    void request_cancel() { pending_cancel_ = true; } // 不直接清零 leaves。
    bool on_cancel_ack() {
        if (!pending_cancel_ || canceled_ || leaves_ == 0) return false;
        // 若待撤期间已全部成交，真实对端不会再把剩余量成功撤销。
        leaves_ = 0; canceled_ = true; pending_cancel_ = false;
        return true; // 仅教学：真实回报还需核对订单与请求 ID。
    }
    int cum() const { return cum_; }
    int leaves() const { return leaves_; }
    int original() const { return original_; }
};

int main() {
    OrderState order;
    order.on_fill(101, 2);  // 已成交 2，余 3。
    order.request_cancel(); // 发请求不等于撤成。
    order.on_fill(102, 1);  // 待撤期间仍可能成交，余 2。
    order.on_fill(102, 1);  // 同一执行 ID 重放，不能再加一。
    order.on_cancel_ack();  // 确认取消当时真正剩下的 2。
    std::cout << order.original() << ',' << order.cum() << ','
              << order.leaves() << std::endl; // 5,3,0。
}
```

## 读后自检

5 手单先成交 2，待撤期间又成交 1，撤成后 CumQty 和 LeavesQty 各是多少？重复的成交报告到来应怎样处理？

这份源码只证明教学模型中的步骤；生产规则和边界见专题详解。
