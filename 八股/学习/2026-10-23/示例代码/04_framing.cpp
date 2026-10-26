// C++17 教学模型：为字节流规定“1 字节长度 + 消息正文”的应用层格式。
// feed 的参数是一块“这次读到的字节”，可能包含半条、一条或多条消息。
#include <cassert>     // assert：验证三次读入后得到的完整消息。
#include <iostream>    // cout：打印已解析的消息。
#include <stdexcept>   // runtime_error：遇到超长消息时报告协议错误。
#include <string>      // string：既保存缓冲，也保存消息正文。
#include <string_view> // string_view：只借用本次输入，不复制整块输入。
#include <vector>      // vector：返回一次 feed 解析出的多条完整消息。

class FrameDecoder {
    std::string pending_; // 保存上次还不足一条完整消息的字节。

public:
    std::vector<std::string> feed(std::string_view chunk) {
        if (!chunk.empty()) pending_.append(chunk.data(), chunk.size()); // 有输入才追加。
        std::vector<std::string> ready;               // 本次完成的消息。
        while (!pending_.empty()) {
            // unsigned char 防止高位字节被有符号 char 解释成负数。
            const auto length = static_cast<unsigned char>(pending_[0]);
            if (length > 4) { // 本例自定最长正文 4 字节，防止无限等待/扩容。
                throw std::runtime_error("frame too long");
            }
            if (pending_.size() < 1u + length) {
                break; // 只有半条：保留 pending_，等待下一次 feed。
            }
            ready.push_back(pending_.substr(1, length)); // 去掉长度字节。
            pending_.erase(0, 1u + length); // 消费这一条，循环处理粘在后面的消息。
        }
        return ready;
    }
};

int main() {
    FrameDecoder decoder; // 同一连接的多次读取必须共用一个解码器。
    const std::string frame1 = std::string(1, static_cast<char>(3)) + "ABC";
    const std::string frame2 = std::string(1, static_cast<char>(2)) + "DE";
    assert(decoder.feed(frame1.substr(0, 2)).empty()); // 长度 + 正文 A。
    auto messages = decoder.feed(frame1.substr(2) + frame2); // BC + 完整第二条。
    assert(messages.size() == 2 && messages[0] == "ABC" && messages[1] == "DE");
    for (const auto& message : messages) { // const 引用只读取，不复制每条消息。
        std::cout << "message: " << message << '\n';
    }
}
