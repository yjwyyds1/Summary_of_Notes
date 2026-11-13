// 目的：求只可向右、向下移动的非负矩形网格最小路径和。
// 前置：网格非空、每行等长且非空；用 long long 避免一般小输入的 int 溢出。
// 步骤：一维 dp[c] 在覆盖前代表上方，dp[c-1] 已代表左方。
#include <algorithm> // std::min：在上方和左方路径中选较小值。
#include <cassert>   // assert：验证多行、单行边界。
#include <stdexcept> // invalid_argument：报告无效网格。
#include <vector>    // vector：保存输入和一维动态规划数组。

long long min_path_sum(const std::vector<std::vector<int>>& grid) {
    if (grid.empty() || grid[0].empty()) throw std::invalid_argument("empty grid");
    const std::size_t cols = grid[0].size();
    std::vector<long long> dp(cols, 0); // dp[c] 存到当前格的最优和。
    for (std::size_t r = 0; r < grid.size(); ++r) {
        if (grid[r].size() != cols) throw std::invalid_argument("ragged grid");
        for (std::size_t c = 0; c < cols; ++c) {
            const long long cost = grid[r][c]; // 一格的代价。
            if (r == 0 && c == 0) dp[c] = cost; // 起点无前驱。
            else if (r == 0) dp[c] = dp[c - 1] + cost; // 首行只能从左来。
            else if (c == 0) dp[c] += cost;            // 首列只能从上来。
            else dp[c] = std::min(dp[c], dp[c - 1]) + cost;
        }
    }
    return dp[cols - 1]; // 终点的最优和。
}

int main() {
    assert(min_path_sum({{1,3,1},{1,5,1},{4,2,1}}) == 7);
    assert(min_path_sum({{1,2,3}}) == 6); // 单行只有一条路径。
}
