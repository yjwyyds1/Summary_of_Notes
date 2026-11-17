#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

int main() {
    std::mutex mutex;
    std::condition_variable changed;
    int value = 0;
    bool ready = false;

    std::thread reader([&] {
        std::unique_lock<std::mutex> lock(mutex);
        while (!ready) {
            // 等的时候会把 mutex 让出来。回来时已重新拿到它。
            changed.wait(lock);
        }
        std::cout << "reader got " << value << '\n';
    });

    {
        std::lock_guard<std::mutex> lock(mutex);
        value = 42;
        ready = true;
    }
    changed.notify_one(); // 状态已更新，提醒一个等待者回来检查。
    reader.join();
}
