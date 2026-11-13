#include <cassert>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

int main() {
    std::packaged_task<int()> task([]() -> int {
        throw std::runtime_error("device error");
    });
    auto result = task.get_future();
    std::thread worker(std::move(task));
    worker.join();

    bool caught = false;
    try {
        (void)result.get();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    assert(caught);
    std::cout << "future OK\n";
}
