// 目的：按一个卖价位上的先后顺序演示部分成交和 leaves 扣减。
// 前置：限价买单只能接受不高于自己限价的卖价；本例不是完整交易所。
// 步骤：用 101×5 的买单依次消费 100×2 与 100×4 的卖单。
#include <algorithm> // std::min 取双方当前可成交数量的较小值。
#include <deque>     // 同价静置订单按进入顺序排队。
#include <iostream>  // 打印每笔成交和剩余量。

struct Resting {
    int id;      // 教学订单号，用来识别是哪一笔静置卖单。
    int price;   // 整数 tick；不要用浮点相等判断是否同价。
    int leaves;  // 该订单尚未成交的数量。
};

int main() {
    const int buy_limit = 101;  // 买方愿付的最高价，不等于必然成交价。
    int buy_leaves = 5;         // incoming 买单的未成交数量。
    std::deque<Resting> asks{{1, 100, 2}, {2, 100, 4}};

    while (buy_leaves > 0 && !asks.empty()) {
        Resting& best = asks.front();  // 同价先处理最早进入的卖单。
        if (best.price > buy_limit) break;  // 超出买方限价就停止。
        const int traded = std::min(buy_leaves, best.leaves);
        buy_leaves -= traded;  // 两方都要扣减，否则会凭空多成交。
        best.leaves -= traded;
        std::cout << "fill seller=" << best.id << " qty=" << traded
                  << " px=" << best.price << std::endl;
        if (best.leaves == 0) asks.pop_front();  // 只在完全成交后移走队首。
    }
    std::cout << "buy_leaves=" << buy_leaves
              << " seller2_leaves=" << (asks.empty() ? 0 : asks.front().leaves)
              << std::endl; // 防止另一组输入把卖盘全部吃空后访问空队列。
}
