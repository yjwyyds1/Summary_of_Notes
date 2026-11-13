# 2026-11-02 · HTTP 长连接与 HTTP/2 多路复用：注释源码

先修：[[学习/2026-11-02/05-连接复用与HTTP2|对应专题]]。此程序为独立 C++17 教学示例，成功时 `assert` 无输出。

```cpp
// 目的：用流 ID 归属演示 HTTP/2 交错传帧的概念，不实现 HTTP/2 协议。
// 前置：输入帧已被可信协议栈校验和解码；真实帧格式、流控、HPACK 均未实现。
// 步骤：按到达顺序接收帧，再按 stream_id 归入各自请求/响应。
#include <cassert> // assert：检查交错帧重组后的概念结果。
#include <map>     // map：按流 ID 找到对应的逻辑消息。
#include <string>  // string：示例载荷，仅用于说明归属。
#include <vector>  // vector：模拟接收顺序。

struct Frame {
    int stream_id;      // 真实 HTTP/2 帧有自己的流标识字段。
    std::string payload; // 教学字符串，不是实际 wire format。
};

int main() {
    const std::vector<Frame> incoming{{1, "A1"}, {3, "B1"}, {1, "A2"}, {3, "B2"}};
    std::map<int, std::string> streams; // 每个流维护各自的已到达内容。
    for (const Frame& frame : incoming) {
        streams[frame.stream_id] += frame.payload; // 只追加到对应流。
    }
    assert(streams.at(1) == "A1A2"); // 流 1 内保持自身顺序。
    assert(streams.at(3) == "B1B2"); // 流 3 可以与流 1 交错到达。
}
```

观察：先自己预测每个断言检查什么，再按 [[学习/2026-11-02/示例验证|验证记录]] 编译运行。
