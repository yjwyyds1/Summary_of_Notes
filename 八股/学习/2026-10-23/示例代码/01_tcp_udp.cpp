// C++17 教学模型：观察 TCP 字节流与 UDP 数据报的应用可见边界。
// 这是内存中的模型，不发送网络数据，也不模拟丢包、重传或操作系统缓冲区。
#include <cassert>  // assert：检查程序理解是否符合预期。
#include <iostream> // cout：把观察结果打印出来。
#include <string>   // string：用文字代表一串字节。
#include <vector>   // vector：保存多条独立数据报。

int main() {
    // 一次发送调用传入 "AB"，另一次传入 "CD"；TCP 接收方只看到连续字节。
    // 真实 recv 每次返回多少字节由当时可读数据和缓冲区大小等决定，
    // 不能根据发送调用次数推断 recv 调用次数。
    const std::string tcp_bytes = std::string("AB") + "CD";
    const std::string first_read = tcp_bytes.substr(0, 1);
    const std::string second_read = tcp_bytes.substr(1, 3);
    assert(first_read == "A" && second_read == "BCD");

    // UDP 则把每次发送的内容作为一条数据报；这里用两个元素表示两条报文。
    // 真实 UDP 可能丢失、乱序；本例只展示“每条报文有边界”。
    const std::vector<std::string> udp_datagrams{"AB", "CD"};
    assert(udp_datagrams.size() == 2);
    std::cout << "TCP reads: [" << first_read << "] [" << second_read << "]\n";
    std::cout << "UDP datagrams: [" << udp_datagrams[0] << "] ["
              << udp_datagrams[1] << "]\n";
}
