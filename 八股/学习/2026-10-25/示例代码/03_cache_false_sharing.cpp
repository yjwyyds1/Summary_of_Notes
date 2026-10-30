// ============================================================================
// 03_cache_false_sharing.cpp
// 目的：用可重复的小实验看两件事——AoS/SoA 的缓存差异、以及伪共享的代价。
// 前置：知道缓存行、顺序访问、以及“测量噪声”这件事（耗时受机器负载影响）。
// 程序步骤：
//   1. 生成 100 万条行情记录，分别按 AoS 与 SoA 存放；
//   2. 只累加 price 字段，比较两者耗时；
//   3. 两个线程各写一个计数器：先放在同一缓存行，再各占一行，比较耗时。
// 观察点：
//   - SoA 只读一列时通常更快，但倍率随机器/编译器/数据量变化，不要背数字；
//   - 伪共享版本明显更慢，且线程越多、竞争越频繁越明显；
//   - 单次测量不可靠，本程序取多轮最小值以减少噪声。
// 编译：g++ -std=c++17 -O2 -pthread 03_cache_false_sharing.cpp -o 03_cache_false_sharing
// ============================================================================

#include <algorithm>   // std::min
#include <atomic>      // std::atomic：伪共享实验里用原子写
#include <chrono>      // 计时
#include <cstddef>     // std::size_t
#include <iostream>    // std::cout
#include <thread>      // std::thread
#include <vector>      // std::vector

namespace {

constexpr std::size_t kCount = 1'000'000;   // 记录条数
constexpr int kRounds = 5;                  // 每项测量轮数，取最小值

// 防止编译器把求和结果优化掉：把结果写进 volatile 变量
volatile double g_sink = 0.0;

// AoS：一条记录三个 double 连着放，共 24 字节
struct QuoteAoS {
    double price;
    double qty;
    double timestamp;
};

// 计时工具：跑 kRounds 轮，返回最小耗时（纳秒）
template <typename Fn>
long long best_ns(Fn&& fn) {
    long long best = (1LL << 62);
    for (int round = 0; round < kRounds; ++round) {
        const auto start = std::chrono::steady_clock::now();
        fn();
        const auto end = std::chrono::steady_clock::now();
        best = std::min(best, std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }
    return best;
}

}  // namespace

// 两个线程各写一个计数器，但两个计数器紧挨着：很可能落在同一条缓存行
struct SharedLine {
    std::atomic<long> first{0};
    std::atomic<long> second{0};
};

// 每个计数器独占一条缓存行（64 字节对齐）
struct alignas(64) PaddedCounter {
    std::atomic<long> value{0};
};

struct PaddedLine {
    PaddedCounter first;
    PaddedCounter second;
};

// 两个线程各自把一个计数器加到 200 万，返回最短耗时
template <typename IncrementFirst, typename IncrementSecond>
long long measure_two_threads(IncrementFirst&& inc_first, IncrementSecond&& inc_second) {
    return best_ns([&] {
        std::thread a([&] { inc_first(); });
        std::thread b([&] { inc_second(); });
        a.join();
        b.join();
    });
}

int main() {
    std::cout << "记录数 " << kCount << "，每项取 " << kRounds << " 轮最小值\n";

    // ------------------------------------------------------------------
    // 1. AoS 与 SoA 的求和对比
    // ------------------------------------------------------------------
    std::vector<QuoteAoS> aos(kCount);
    std::vector<double> soa_price(kCount);
    std::vector<double> soa_qty(kCount);
    std::vector<double> soa_timestamp(kCount);

    for (std::size_t i = 0; i < kCount; ++i) {
        const double price = 100.0 + static_cast<double>(i % 100) * 0.01;
        aos[i] = QuoteAoS{price, 1.0, static_cast<double>(i)};
        soa_price[i] = price;
        soa_qty[i] = 1.0;
        soa_timestamp[i] = static_cast<double>(i);
    }

    const long long aos_ns = best_ns([&aos] {
        double sum = 0.0;
        for (const QuoteAoS& quote : aos) {
            sum += quote.price;          // 只读一列，却把 24 字节都搬进缓存
        }
        g_sink = sum;
    });

    const long long soa_ns = best_ns([&soa_price] {
        double sum = 0.0;
        for (double price : soa_price) {  // 连续访问，全部是要用的数据
            sum += price;
        }
        g_sink = sum;
    });

    std::cout << "\n=== AoS vs SoA（只累加 price）===\n";
    std::cout << "AoS 用时 " << aos_ns / 1000 << " us\n";
    std::cout << "SoA 用时 " << soa_ns / 1000 << " us\n";
    std::cout << "比值 AoS/SoA = " << static_cast<double>(aos_ns) / static_cast<double>(soa_ns) << "\n";

    // ------------------------------------------------------------------
    // 2. 伪共享对比
    // ------------------------------------------------------------------
    SharedLine shared;
    PaddedLine padded;

    const long long shared_ns = measure_two_threads(
        [&shared] {
            for (int i = 0; i < 2'000'000; ++i) {
                shared.first.fetch_add(1, std::memory_order_relaxed);
            }
        },
        [&shared] {
            for (int i = 0; i < 2'000'000; ++i) {
                shared.second.fetch_add(1, std::memory_order_relaxed);
            }
        });

    const long long padded_ns = measure_two_threads(
        [&padded] {
            for (int i = 0; i < 2'000'000; ++i) {
                padded.first.value.fetch_add(1, std::memory_order_relaxed);
            }
        },
        [&padded] {
            for (int i = 0; i < 2'000'000; ++i) {
                padded.second.value.fetch_add(1, std::memory_order_relaxed);
            }
        });

    std::cout << "\n=== 伪共享（两线程各写一个计数器）===\n";
    std::cout << "同一缓存行：" << shared_ns / 1000 << " us\n";
    std::cout << "各占一行  ：" << padded_ns / 1000 << " us\n";
    std::cout << "比值 同一行/分行 = " << static_cast<double>(shared_ns) / static_cast<double>(padded_ns) << "\n";
    std::cout << "注意：同机同编译选项下的相对结果，换机器/负载会变，不要当固定倍数\n";
    return 0;
}
