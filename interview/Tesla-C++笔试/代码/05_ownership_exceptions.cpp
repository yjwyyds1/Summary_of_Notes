#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

struct Session {
    int volume;
};

class Player {
public:
    void configure(int volume) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (volume < 0 || volume > 100) throw std::invalid_argument("volume");
        auto next = std::make_unique<Session>(Session{volume});
        session_ = std::move(next); // Commit after validation and allocation.
    }

    int volume() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return session_->volume;
    }

private:
    mutable std::mutex mutex_;
    std::unique_ptr<Session> session_ = std::make_unique<Session>(Session{0});
};

int main() {
    Player player;
    player.configure(35);
    bool caught = false;
    try {
        player.configure(-1);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught && player.volume() == 35);

    auto active = std::make_shared<const Session>(Session{80});
    std::weak_ptr<const Session> observer = active;
    assert(observer.lock()->volume == 80);
    active.reset();
    assert(!observer.lock());
    std::cout << "ownership and exceptions OK\n";
}
