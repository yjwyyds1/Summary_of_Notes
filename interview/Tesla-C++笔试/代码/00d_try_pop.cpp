#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class Inbox {
public:
    void push(int value) {
        std::lock_guard<std::mutex> guard(mutex_);
        numbers_.push(value);
    }

    bool tryPop(int& value) {
        std::lock_guard<std::mutex> guard(mutex_);
        if (numbers_.empty()) {
            return false;
        }
        value = numbers_.front();
        numbers_.pop();
        return true;
    }

private:
    std::mutex mutex_;
    std::queue<int> numbers_;
};

int main() {
    Inbox inbox;
    int countA = 0;
    int countB = 0;
    for (int i = 0; i < 100; ++i) {
        inbox.push(i);
    }

    std::thread readerA([&] {
        int value = 0;
        while (inbox.tryPop(value)) {
            ++countA;
        }
    });

    std::thread readerB([&] {
        int value = 0;
        while (inbox.tryPop(value)) {
            ++countB;
        }
    });

    readerA.join();
    readerB.join();
    std::cout << "total = " << countA + countB << '\n';
}
