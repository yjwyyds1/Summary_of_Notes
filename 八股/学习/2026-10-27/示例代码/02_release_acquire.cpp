// 目的：演示一次性消息交接的 release/acquire 同步链。
// 前提：生产者发布后不再改普通 payload；主线程等待两线程结束。
// 观察：消费者只在 acquire 读到 true 后读取两个普通字段。
#include <atomic>     // std::atomic<bool>：线程之间的发布标志。
#include <iostream>   // std::cout：输出消费者观察到的消息。
#include <thread>     // std::thread、std::this_thread::yield。

struct Message { int price{0}; int qty{0}; }; // 普通对象，不是 atomic。

int main() {
    Message message;
    std::atomic<bool> ready{false};
    std::thread producer([&] { // 按引用捕获主线程中仍存活的对象。
        message.price = 42; // 先完成普通写。
        message.qty = 3;
        ready.store(true, std::memory_order_release); // 再发布完成信号。
    });
    std::thread consumer([&] {
        while (!ready.load(std::memory_order_acquire))
            std::this_thread::yield(); // 教学演示；生产服务应设计等待策略。
        // acquire 读到发布的 true 后，这里才能读普通字段。
        std::cout << "price=" << message.price << " qty=" << message.qty << '\n';
    });
    producer.join();
    consumer.join();
}
