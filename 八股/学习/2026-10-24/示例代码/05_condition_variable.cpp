// C++17 条件变量：消费者在“还没有数据”时睡眠，生产者写数据后发通知。
// 使用谓词版 wait，避免虚假唤醒或通知先到造成逻辑错误。
#include <cassert>            // assert：检查消费者最终拿到的值。
#include <condition_variable> // condition_variable：等待与通知。
#include <iostream>           // cout：展示结果。
#include <mutex>              // mutex、unique_lock、lock_guard：保护共享状态。
#include <thread>             // thread：创建消费者线程。

int main() {
    std::mutex mutex;                 // ready 和 value 都受同一把锁保护。
    std::condition_variable changed; // 通知“共享状态可能变化了”。
    bool ready = false;              // 真正的条件：数据是否已经准备好。
    int value = 0;
    int consumed = 0; // 只由工作线程写；join 后主线程才读。

    std::thread consumer([&] {
        std::unique_lock<std::mutex> lock(mutex); // wait 需要可解锁/重锁的锁对象。
        changed.wait(lock, [&] { return ready; }); // 等待时释放锁，醒后重锁并重新查 ready。
        consumed = value; // 此时持锁，读到与 ready 一起发布的值。
    });

    {
        std::lock_guard<std::mutex> lock(mutex); // 作用域结束自动解锁。
        value = 42;
        ready = true; // 先改变条件，再通知；谓词决定是否继续等待。
    }
    changed.notify_one(); // 唤醒一个等待者；即使先于 wait，ready 仍能被检查。
    consumer.join();
    assert(consumed == 42);
    std::cout << "consumed: " << consumed << '\n';
}
