#include <iostream>
#include <mutex>
#include <thread>

void addTwice(int& count, std::mutex& mutex) {
    for (int i = 0; i < 2; ++i) {
        std::lock_guard<std::mutex> lock(mutex); // 进作用域拿锁。
        ++count;
    } // 每次循环结束，lock 销毁并解锁。
}

int main() {
    int count = 0;
    std::mutex mutex;

    // [&] 让两个线程都使用 main 里的同一个 count 和同一把锁。
    std::thread first([&] { addTwice(count, mutex); });
    std::thread second([&] { addTwice(count, mutex); });
    first.join();
    second.join();

    // 两个线程已结束，这里直接读 count。
    std::cout << "count = " << count << '\n'; // 一定是 4。
}
