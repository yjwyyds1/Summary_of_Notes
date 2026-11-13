# 2026-11-02 · HTTPS 怎样建立加密且可认证的连接：注释源码

先修：[[学习/2026-11-02/06-HTTPS与TLS握手|对应专题]]。此程序为独立 C++17 教学示例，成功时 `assert` 无输出。

```cpp
// 目的：仅演示“认证完成才放行 HTTP 数据”的应用状态门槛。
// 前置：真实证书、密钥交换和加解密必须交给成熟 TLS 库；本代码绝不执行密码学操作。
// 步骤：先检查协商与身份验证状态，再允许调用方进入发送阶段。
#include <cassert>   // assert：验证失败状态不会被误当作可发送。
#include <stdexcept> // logic_error：阻止未认证连接承载请求。

struct TlsState {
    bool handshake_finished = false; // TLS 库报告握手已经完成。
    bool certificate_valid = false;  // TLS 库报告链与有效期核验成功。
    bool hostname_matches = false;   // TLS 库报告证书身份匹配目标域名。

    bool authenticated() const {
        return handshake_finished && certificate_valid && hostname_matches;
    }
    void require_before_http() const {
        if (!authenticated()) throw std::logic_error("TLS peer not authenticated");
    }
};

int main() {
    TlsState state;
    bool blocked = false;
    try { state.require_before_http(); } // 握手前不得发应用请求。
    catch (const std::logic_error&) { blocked = true; }
    assert(blocked);
    state.handshake_finished = true; // 这里只模拟 TLS 库回报，不是假装自己握手。
    state.certificate_valid = true;
    state.hostname_matches = true;
    state.require_before_http();     // 三项都满足后才通过。
}
```

观察：先自己预测每个断言检查什么，再按 [[学习/2026-11-02/示例验证|验证记录]] 编译运行。
