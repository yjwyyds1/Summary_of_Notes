#include <cassert>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>

using namespace std::chrono_literals;

int main() {
    std::timed_mutex gate;
    std::promise<void> locked;
    std::promise<void> release;
    auto lockedSignal = locked.get_future();
    auto releaseSignal = release.get_future();
    std::thread holder([&] {
        std::lock_guard<std::timed_mutex> lock(gate);
        locked.set_value();
        releaseSignal.wait();
    });
    lockedSignal.wait();
    std::unique_lock<std::timed_mutex> attempt(gate, std::defer_lock);
    const bool acquired = attempt.try_lock_for(2ms);
    release.set_value();
    holder.join();
    assert(!acquired);

    std::mutex eventMutex;
    std::condition_variable event;
    bool ready = false;
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        const bool signalled = event.wait_for(lock, 2ms, [&] { return ready; });
        assert(!signalled);
        ready = true;
        assert(event.wait_for(lock, 0ms, [&] { return ready; }));
    }

    std::mutex left, right;
    {
        std::scoped_lock lock(left, right);
        // Both resources can be changed as one operation.
    }
    {
        std::unique_lock first(left, std::defer_lock);
        std::unique_lock second(right, std::defer_lock);
        std::lock(first, second);
        assert(first.owns_lock() && second.owns_lock());
    }

    std::shared_mutex settings;
    int volume = 0;
    {
        std::unique_lock writer(settings);
        volume = 25;
    }
    {
        std::shared_lock reader(settings);
        assert(volume == 25);
    }

    std::recursive_mutex legacy;
    {
        std::lock_guard outer(legacy);
        std::lock_guard inner(legacy);
    }
    std::cout << "timeout and locks OK\n";
}
