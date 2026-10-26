// C++17 教学模型：接收窗口 rwnd 与拥塞窗口 cwnd 一起限制未确认数据。
// 数字单位简化为“字节”；真实 TCP 还会处理序号回绕、重传和窗口更新。
#include <algorithm> // min：取多个限制中的最小值。
#include <cassert>   // assert：验证不会超过可发送额度。
#include <iostream>  // cout：打印每一步的额度。

int available(int rwnd, int cwnd, int in_flight) {
    // rwnd 表示接收端告知的空间；cwnd 表示发送端估计网络可承受的量。
    // in_flight 是已经发出、仍未被确认的字节数。
    return std::max(0, std::min(rwnd, cwnd) - in_flight);
}

int main() {
    int rwnd = 8;      // 接收方广告：目前可容纳 8 字节。
    int cwnd = 5;      // 拥塞控制：目前最多容许 5 字节在途。
    int in_flight = 2; // 已发出的 2 字节还没收到 ACK。
    assert(available(rwnd, cwnd, in_flight) == 3);
    std::cout << "before ACK: " << available(rwnd, cwnd, in_flight) << '\n';

    in_flight -= 2; // 收到这 2 字节的确认，才释放在途额度。
    rwnd = 1;       // 接收方应用处理慢，新通知的可用窗口只剩 1。
    assert(available(rwnd, cwnd, in_flight) == 1);
    std::cout << "receiver slow: " << available(rwnd, cwnd, in_flight) << '\n';

    rwnd = 8;      // 接收方处理完毕，允许的在途额度再次增大。
    cwnd = 1;      // 网络侧窗口收紧，此时只允许 1 字节在途。
    in_flight = 1; // 已有 1 字节在途，所以没有新增发送额度。
    assert(available(rwnd, cwnd, in_flight) == 0);
    std::cout << "network limit: " << available(rwnd, cwnd, in_flight) << '\n';
}
