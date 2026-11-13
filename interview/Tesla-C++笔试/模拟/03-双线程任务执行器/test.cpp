#include "executor.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

void check(bool passed, const char* label) {
    if (!passed) throw std::runtime_error(label);
    std::cout << "PASS " << label << '\n';
}

int main(int argc, char** argv) try {
    const std::string test = argc > 1 ? argv[1] : "all";
    if (test == "all" || test == "result") {
        Executor executor;
        auto ok = executor.submit([] { return 42; });
        check(ok.valid() && ok.get() == 42, "result");
        auto failure = executor.submit([]() -> int { throw std::invalid_argument("device"); });
        bool caught = false;
        try { (void)failure.get(); }
        catch (const std::invalid_argument&) { caught = true; }
        check(caught, "preserve exception type");
        check(executor.submit([] { return 8; }).get() == 8, "worker still usable");
    }
    if (test == "all" || test == "workers") {
        std::promise<void> oneStarted, twoStarted, release;
        auto one = oneStarted.get_future();
        auto two = twoStarted.get_future();
        auto gate = release.get_future().share();
        Executor executor;
        auto a = executor.submit([&] { oneStarted.set_value(); gate.wait(); return 1; });
        auto b = executor.submit([&] { twoStarted.set_value(); gate.wait(); return 2; });
        const bool both = one.wait_for(1s) == std::future_status::ready &&
                          two.wait_for(1s) == std::future_status::ready;
        release.set_value();
        check(both, "two tasks can run simultaneously");
        check(a.get() == 1 && b.get() == 2, "both results");
    }
    if (test == "all" || test == "stop") {
        Executor executor;
        std::vector<std::future<int>> accepted;
        for (int i = 0; i < 80; ++i)
            accepted.push_back(executor.submit([i] { return i; }));

        std::vector<std::future<int>> concurrent;
        std::thread producer([&] {
            for (int i = 80; i < 160; ++i) {
                try { concurrent.push_back(executor.submit([i] { return i; })); }
                catch (const std::runtime_error&) { return; }
            }
        });
        executor.stop();
        producer.join();
        executor.stop();
        for (int i = 0; i < 80; ++i)
            if (!accepted[i].valid() || accepted[i].get() != i) throw std::runtime_error("drain queued task");
        for (std::size_t i = 0; i < concurrent.size(); ++i)
            if (concurrent[i].get() != 80 + static_cast<int>(i)) throw std::runtime_error("drain concurrent task");
        std::cout << "PASS drain accepted tasks\n";
        bool rejected = false;
        try { (void)executor.submit([] { return 0; }); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "reject after stop");

        std::future<int> last;
        { Executor scoped; last = scoped.submit([] { return 5; }); }
        check(last.valid() && last.get() == 5, "destructor drains");
    }
} catch (const std::exception& e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
}
