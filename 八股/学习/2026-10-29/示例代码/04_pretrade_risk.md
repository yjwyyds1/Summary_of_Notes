# 下单前风控、额度与重复请求 · 注释源码阅读版

先修：[[学习/2026-10-29/04-下单前风控与重复请求|专题详解]]。以下与保存的 cpp 和专题代码块逐字一致。

```cpp
// 目的：演示单线程教学版下单前风控与本地重复 ID 检查。
// 前置：名义金额=整数价格×数量；实际产品还需乘数、币种和市场规则。
// 步骤：先接受 50，再拒绝超额 60 和重复 ID。
#include <iostream>      // 打印三次提交结果。
#include <unordered_set> // 保存已接受的客户端订单 ID。

class RiskGate {
    int available_ = 100;        // 教学额度，不代表真实限额。
    int max_single_qty_ = 6;     // 单笔数量上限。
    bool halted_ = false;        // 停发开关；真实系统需受控更新。
    std::unordered_set<int> ids_; // 本进程内去重，重启后不会自动保留。
public:
    bool submit(int id, int price, int qty) {
        if (halted_ || price <= 0 || qty <= 0 || qty > max_single_qty_)
            return false; // 输入无效或已停发。
        if (ids_.count(id)) return false; // 活跃教学记录中不允许重复 ID。
        if (price > available_ / qty) return false; // 除法检查避免乘积溢出。
        const int notional = price * qty; // 上面的条件保证不超过 available_。
        available_ -= notional; // 接受后立即预留，下一笔看到剩余额度。
        ids_.insert(id);        // 与预留同在单线程受控路径里更新。
        return true;
    }
    int available() const { return available_; }
};

int main() {
    RiskGate gate;
    std::cout << gate.submit(1, 10, 5) << std::endl; // 1，预留 50。
    std::cout << gate.submit(2, 10, 6) << std::endl; // 0，剩余 50 不够 60。
    std::cout << gate.submit(1, 10, 1) << std::endl; // 0，重复 ID。
    std::cout << gate.available() << std::endl;      // 仍为 50。
}
```

## 读后自检

额度 100，先接受 50，再来 60 为什么拒绝？如果第一笔超时，能直接换新 ID 再发吗？

这份源码只证明教学模型中的步骤；生产规则和边界见专题详解。
