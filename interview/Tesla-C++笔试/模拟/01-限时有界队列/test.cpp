#include "queue.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std::chrono_literals;

void check(bool passed, const char* label) {
    if (!passed) throw std::runtime_error(label);
    std::cout << "PASS " << label << '\n';
}

int main(int argc, char** argv) try {
    const std::string test = argc > 1 ? argv[1] : "all";
    if (test == "all" || test == "fifo") {
        bool rejected = false;
        try { BoundedQueue invalid(0); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "zero capacity");
        BoundedQueue queue(2);
        check(queue.pushFor(0, 0ms) == PushResult::accepted, "accept zero");
        check(queue.pushFor(9, 0ms) == PushResult::accepted, "fill");
        check(queue.pop() == 0 && queue.pop() == 9, "FIFO");
    }
    if (test == "all" || test == "full") {
        BoundedQueue queue(1);
        check(queue.pushFor(10, 0ms) == PushResult::accepted, "fill slot");
        check(queue.pushFor(20, 5ms) == PushResult::timeout, "full timeout");
        std::promise<void> started;
        auto signal = started.get_future();
        auto writer = std::async(std::launch::async, [&] {
            started.set_value();
            return queue.pushFor(20, 2s);
        });
        signal.wait();
        const auto first = queue.pop();
        check(first == 10, "pop frees slot");
        check(writer.get() == PushResult::accepted, "writer succeeds");
        check(queue.pop() == 20, "new value");
    }
    if (test == "all" || test == "close") {
        BoundedQueue queue(1);
        check(queue.pushFor(10, 0ms) == PushResult::accepted, "fill before close");
        std::promise<void> started;
        auto signal = started.get_future();
        auto writer = std::async(std::launch::async, [&] {
            started.set_value();
            return queue.pushFor(20, 1h);
        });
        signal.wait();
        queue.close();
        queue.close();
        check(writer.get() == PushResult::closed, "closing releases writer");
        check(queue.pop() == 10 && !queue.pop(), "drain before end");
        check(queue.pushFor(30, 0ms) == PushResult::closed, "reject closed");
        BoundedQueue empty(1);
        std::promise<void> reading;
        auto readerStarted = reading.get_future();
        auto reader = std::async(std::launch::async, [&] {
            reading.set_value();
            return empty.pop();
        });
        readerStarted.wait();
        empty.close();
        check(!reader.get(), "closing releases reader");
    }
} catch (const std::exception& e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
}
