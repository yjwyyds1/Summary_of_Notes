// C++17 教学模型：区分读到数据、暂时无数据、对端关闭与真正错误。
// 本程序不调用 recv；Linux 实际返回值与 errno 的映射见专题页和 recv(2)。
#include <cassert>  // assert：验证处理结果。
#include <iostream> // cout：展示分支走向。
#include <string>   // string：模拟读到的字节。

enum class ReadKind { data, would_block, peer_closed, failed };

struct ReadResult {
    ReadKind kind;    // 本次读取属于哪种情况。
    std::string bytes; // 只有 data 分支才使用这里的内容。
};

std::string next_action(const ReadResult& result) {
    // const & 表示借用且不修改 result，避免复制 string。
    switch (result.kind) {
    case ReadKind::data:
        return "parse " + result.bytes; // 正数字节：送入协议解析器。
    case ReadKind::would_block:
        return "wait for readiness"; // EAGAIN：不是永久错误；等待新就绪。
    case ReadKind::peer_closed:
        return "close connection"; // TCP recv 返回 0：对端有序关闭发送方向。
    case ReadKind::failed:
        return "report error"; // 其它失败应记录 errno 和具体上下文。
    }
    return "unreachable";
}

int main() {
    assert(next_action({ReadKind::would_block, ""}) == "wait for readiness");
    std::cout << next_action({ReadKind::data, "ABC"}) << '\n';
    std::cout << next_action({ReadKind::would_block, ""}) << '\n';
    std::cout << next_action({ReadKind::peer_closed, ""}) << '\n';
}
