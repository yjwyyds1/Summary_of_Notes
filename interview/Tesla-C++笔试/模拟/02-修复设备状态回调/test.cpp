#include "device.hpp"
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

void check(bool passed, const char* label) {
    if (!passed) throw std::runtime_error(label);
    std::cout << "PASS " << label << '\n';
}

int main(int argc, char** argv) try {
    const std::string test = argc > 1 ? argv[1] : "all";
    if (test == "all" || test == "state") {
        Device device;
        check(device.setVolume(100), "accept upper boundary");
        check(!device.setVolume(-1) && !device.setVolume(101), "reject invalid");
        const auto state = device.snapshot();
        check(state.volume == 100 && state.revision == 1, "invalid leaves state alone");
        check(device.setVolume(100) && device.snapshot().revision == 2, "same value commits");
    }
    if (test == "all" || test == "reentry") {
        Device device;
        int observed = 0;
        device.setCallback([&](DeviceState state) {
            if (device.snapshot().revision == state.revision) observed = state.volume;
            device.setCallback({});
        });
        check(device.setVolume(42) && observed == 42, "reentrant callback sees committed snapshot");
        check(device.setVolume(1) && observed == 42, "callback can unregister");
    }
    if (test == "all" || test == "exception") {
        Device device;
        device.setCallback([](DeviceState) { throw std::runtime_error("callback failed"); });
        bool threw = false;
        try { (void)device.setVolume(20); }
        catch (const std::runtime_error&) { threw = true; }
        check(threw && device.snapshot().revision == 1, "exception propagates after commit");
    }
    if (test == "all" || test == "concurrent") {
        Device device;
        std::atomic_int calls{0};
        device.setCallback([&](DeviceState state) {
            if (state.revision > 0) calls.fetch_add(1);
        });
        const auto update = [&] {
            for (int i = 0; i < 100; ++i) (void)device.setVolume(i);
        };
        std::thread a(update), b(update);
        a.join();
        b.join();
        check(device.snapshot().revision == 200 && calls.load() == 200, "two writers");
    }
} catch (const std::exception& e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
}
