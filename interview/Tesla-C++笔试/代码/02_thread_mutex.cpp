#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>

class Seats {
public:
    bool reserve() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (free_ == 0) return false;
        --free_;
        ++used_;
        return true;
    }

    int used() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return used_;
    }

private:
    mutable std::mutex mutex_;
    int free_ = 100;
    int used_ = 0;
};

int main() {
    Seats seats;
    const auto reserveMany = [&] {
        for (int i = 0; i < 80; ++i) seats.reserve();
    };
    std::thread first(reserveMany);
    std::thread second(reserveMany);
    first.join();
    second.join();
    assert(seats.used() == 100);
    std::cout << "thread and mutex OK\n";
}
