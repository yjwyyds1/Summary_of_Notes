// 目的：同一组行情数据分别以 AoS 与 SoA 表示，验证按价格求和一致。
// 前置：vector、结构体和下标；编译标准 C++17。
#include <cassert>  // assert：验证结果与列长。
#include <iostream> // cout：显示求和。
#include <vector>   // vector：保存连续元素。

struct Tick { int id; int price; int volume; }; // AoS 的一条完整记录。
struct TickColumns {                         // SoA：每个字段单独一列。
    std::vector<int> ids;
    std::vector<int> prices;
    std::vector<int> volumes;
};

int main() {
    std::vector<Tick> rows{{1, 101, 5}, {2, 103, 8}};
    TickColumns cols{{1, 2}, {101, 103}, {5, 8}};
    assert(cols.ids.size() == cols.prices.size()); // 第 i 项必须对应同一条。
    assert(cols.prices.size() == cols.volumes.size());
    int aos_sum = 0;
    for (const Tick& t : rows) aos_sum += t.price; // 只用 price 字段。
    int soa_sum = 0;
    for (int price : cols.prices) soa_sum += price; // 只遍历价格列。
    assert(aos_sum == soa_sum);
    std::cout << "price sum: " << soa_sum << '\n';
    // 此程序只验证语义，不能据一次运行证明某种布局更快。
}
