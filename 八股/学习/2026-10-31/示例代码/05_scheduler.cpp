// 用途：用整数时钟模拟 RR（时间片轮转），看“首次运行”和“完成”怎样不同。
// 先修：vector 保存对象；deque 在两端入队/出队；const & 表示只读借用。
// 模型：单 CPU，任务全在时刻 0 到达，只计算 CPU 时间，不含 I/O 或切换成本。
// 本程序没有创建线程、没有设置 OS 调度策略，不是实际 Linux 调度器。
// 步骤：建立就绪队列 -> 每次取队头运行一片 -> 未完成者回到队尾 -> 输出指标。
// 观察：A/B/C 的首次运行是 0/2/4，完成时刻却是 9/8/5。

#include <algorithm>  // std::min：本次最多运行一个时间片，也不能超过剩余量。
#include <cassert>    // assert：教学核对；编译时不要定义 NDEBUG，否则断言会关闭。
#include <cstddef>    // std::size_t：容器下标的无符号整数类型。
#include <cstdint>    // std::int64_t：用较宽整数记录累计模拟时刻。
#include <deque>      // std::deque：保存“下一位轮到谁”的下标队列。
#include <iostream>   // std::cout：输出模拟时间线，并非真实运行耗时。
#include <stdexcept>  // std::invalid_argument：拒绝不合法的教学输入。
#include <vector>     // std::vector<T>：尖括号中的 T 指容器保存的元素类型。

struct Job {
    char name;   // 仅供显示的任务名；调度时用下标区分任务。
    int burst;   // 总共需要多少单位 CPU 时间，必须大于 0。
};

struct Slice {
    std::size_t task;       // 本片属于 jobs 中的哪个任务。
    std::int64_t begin;     // 半开时间区间 [begin, end) 的开始。
    std::int64_t end;       // 到达 end 时，这一片已经运行完毕。
};

struct Result {
    std::vector<std::int64_t> first;   // -1 表示这个任务还从未运行。
    std::vector<std::int64_t> finish;  // -1 表示这个任务尚未完成。
    std::vector<Slice> timeline;       // 依次保存每一片，便于手算核对。
};

// jobs 是只读引用，不复制输入；quantum 是每次最多使用的 CPU 时间。
// 返回值拥有结果数据。函数只适合规模很小的教学模拟，不用于生产调度。
Result round_robin(const std::vector<Job>& jobs, int quantum) {
    if (quantum <= 0) {
        throw std::invalid_argument("quantum must be positive");
    }
    Result result;
    result.first.assign(jobs.size(), -1);  // 每个任务都分到一个初始标记。
    result.finish.assign(jobs.size(), -1);
    std::vector<int> remaining;           // 剩余 CPU 时间，与 jobs 的下标对应。
    std::deque<std::size_t> ready;        // 队列中只放下标，不复制整个任务。
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        if (jobs[i].burst <= 0) {
            throw std::invalid_argument("burst must be positive");
        }
        remaining.push_back(jobs[i].burst);
        ready.push_back(i);  // 同时到达时，用输入顺序 A、B、C 打破平局。
    }

    std::int64_t now = 0;    // 模拟时钟；没有调用 sleep，也不读取墙上时钟。
    while (!ready.empty()) {
        const std::size_t i = ready.front();
        ready.pop_front();  // 当前任务先离开就绪队列，转成“正在运行”。
        if (result.first[i] == -1) {
            result.first[i] = now;  // 只记第一次；后续轮转不能覆盖它。
        }
        const int run = std::min(quantum, remaining[i]);
        result.timeline.push_back({i, now, now + run});  // 花括号按成员顺序初始化。
        now += run;          // 时间只增加真正执行的量，最后一片可能不足 quantum。
        remaining[i] -= run;
        if (remaining[i] == 0) {
            result.finish[i] = now;  // 完成者不再入队，否则会永远转下去。
        } else {
            ready.push_back(i);     // 没做完：让其他已就绪任务先轮到 CPU。
        }
    }
    return result;
}

int main() {
    const std::vector<Job> jobs{{'A', 5}, {'B', 3}, {'C', 1}};
    const Result result = round_robin(jobs, 2);

    // 先核对手算结果，再打印。双括号使 assert 宏不会误分 initializer_list 中的逗号。
    assert((result.first == std::vector<std::int64_t>{0, 2, 4}));
    assert((result.finish == std::vector<std::int64_t>{9, 8, 5}));
    const std::vector<Slice> expected{{0, 0, 2}, {1, 2, 4}, {2, 4, 5},
                                      {0, 5, 7}, {1, 7, 8}, {0, 8, 9}};
    assert(result.timeline.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(result.timeline[i].task == expected[i].task);
        assert(result.timeline[i].begin == expected[i].begin);
        assert(result.timeline[i].end == expected[i].end);
    }
    assert(round_robin({}, 2).timeline.empty());       // 没任务时应立刻结束。
    assert(round_robin({{'X', 1}}, 5).finish[0] == 1); // 不足一个时间片也只运行 1。
    bool rejected = false;
    try {
        round_robin(jobs, 0);  // 零时间片会让模拟无法前进，所以明确拒绝。
    } catch (const std::invalid_argument&) {  // const & 只读借用异常对象。
        rejected = true;
    }
    assert(rejected);

    std::cout << "RR quantum=2\n";
    for (const Slice& slice : result.timeline) {  // & 借用已有片段，避免逐项复制。
        std::cout << jobs[slice.task].name << " [" << slice.begin
                  << "," << slice.end << ")\n";
    }
    std::cout << "task response turnaround waiting\n";
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        // 全部到达时刻为 0，因此 response=first、turnaround=finish。
        // 只有“无 I/O、无切换成本”的本模型里，waiting=turnaround-burst。
        std::cout << jobs[i].name << ' ' << result.first[i] << ' '
                  << result.finish[i] << ' ' << result.finish[i] - jobs[i].burst << '\n';
    }
    std::cout << "checks passed\n";
}
