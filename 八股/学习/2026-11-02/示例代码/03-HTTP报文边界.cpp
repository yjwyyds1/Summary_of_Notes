// 目的：只演示 HTTP/1.1 固定 Content-Length 请求在分批到达时的完成判断。
// 前置：教学用严格子集；输入恰有一个规范字段，不支持 chunked、重复字段和完整 HTTP 语法。
// 步骤：找头部结束位置，读十进制长度，判断正文是否收齐；多余字节仍留在缓冲中。
#include <cassert>    // assert：验证半条与完整两种状态。
#include <cctype>     // isdigit：只接受十进制长度字符。
#include <stdexcept>  // invalid_argument：拒绝本示例不支持的格式。
#include <string>     // string：保存跨次读取的字节。

std::size_t complete_prefix(const std::string& bytes) {
    const auto end = bytes.find("\r\n\r\n"); // 字段区与正文之间的空行。
    if (end == std::string::npos) return 0;  // 头还没收齐。
    const std::string header = bytes.substr(0, end + 4);
    if (header.find("Transfer-Encoding:") != std::string::npos)
        throw std::invalid_argument("unsupported transfer coding");
    const std::string key = "\r\nContent-Length: ";
    const auto start = header.find(key);
    if (start == std::string::npos) throw std::invalid_argument("missing length");
    const auto value = start + key.size();
    const auto line_end = header.find("\r\n", value);
    if (line_end == value || line_end == std::string::npos)
        throw std::invalid_argument("bad length");
    std::size_t length = 0;
    for (auto i = value; i < line_end; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(header[i])))
            throw std::invalid_argument("non-decimal length");
        const auto digit = static_cast<std::size_t>(header[i] - '0');
        if (length > (1024 - digit) / 10) throw std::invalid_argument("too large");
        length = length * 10 + digit; // 教学示例把正文上限设为 1024 字节。
    }
    const auto total = end + 4 + length; // 消息总长度 = 头部加正文。
    return bytes.size() >= total ? total : 0;
}

int main() {
    std::string buffer = "POST /x HTTP/1.1\r\nHost: example.test\r\nContent-Length: 5\r\n\r\nabc";
    assert(complete_prefix(buffer) == 0); // 只收到 3/5 个正文八位字节。
    buffer += "deNEXT";                  // 下一次还可能带来下条消息开头。
    const auto n = complete_prefix(buffer);
    assert(n > 0 && buffer.substr(n) == "NEXT"); // 多余字节不能丢。
}
