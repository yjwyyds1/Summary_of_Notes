// C++17 教学模型：按 RFC 9293 的普通主动打开与正常关闭画出消息顺序。
// 仅展示常见路径；不是 TCP 协议栈，未模拟重传、同时打开/关闭和 RST。
#include <cassert>  // assert：核对 ACK 与序号的关系。
#include <iostream> // cout：输出可口述的过程。

int main() {
    // SYN 表示同步初始序号；它会占用一个序号，所以确认号是 x+1。
    const int client_isn = 100; // 客户端初始序号，数字仅供举例。
    const int server_isn = 900; // 服务端自己的初始序号。
    const int server_ack = client_isn + 1;
    const int client_ack = server_isn + 1;
    assert(server_ack == 101 && client_ack == 901);

    std::cout << "1 client -> server: SYN seq=" << client_isn << '\n';
    std::cout << "2 server -> client: SYN+ACK seq=" << server_isn
              << " ack=" << server_ack << '\n';
    std::cout << "3 client -> server: ACK ack=" << client_ack << '\n';

    // 双向连接可分别关闭发送方向。A 的 FIN 被确认后，B 仍可发送数据。
    // 这里列出常见的四个报文；ACK 与 FIN 在某些情况下可合并，
    // 因此“四次挥手”不是所有连接结束路径都固定出现四个物理报文。
    std::cout << "4 A -> B: FIN (A stops sending)\n";
    std::cout << "5 B -> A: ACK (B may still send)\n";
    std::cout << "6 B -> A: FIN (B stops sending)\n";
    std::cout << "7 A -> B: ACK (A enters TIME-WAIT)\n";
}
