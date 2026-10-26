# 06_thread_pool — 完整注释源码

← [[学习/2026-10-24/示例代码/README|代码索引]] · [[学习/2026-10-24/06-线程池|主题详解]]

以下代码与同名 .cpp 逐字一致；完整编译与输出见 [[学习/2026-10-24/示例验证]]。

```cpp
// C++17 最小线程池：两条工作线程复用、一个任务队列、条件变量唤醒、析构时收尾。
// 教学版不支持返回值、取消、队列上限或任务异常；不要直接当作生产级实现。
#include <cassert>            // assert：检查任务都完成。
#include <condition_variable> // condition_variable：队列空时让工作线程睡眠。
#include <functional>         // function<void()>：保存不同类型的无参数任务。
#include <iostream>           // cout：展示完成数。
#include <mutex>              // mutex/锁：保护队列、停机标志和结果。
#include <queue>              // queue：先进先出保存待处理任务。
#include <stdexcept>          // runtime_error：拒绝停机后的提交。
#include <thread>             // thread：长期存在的工作线程。
#include <utility>            // move：把任务移进/移出队列。
#include <vector>             // vector：保存工作线程对象。

class ThreadPool {
    std::mutex mutex_;                 // 保护 jobs_ 与 stopping_。
    std::condition_variable changed_; // 新任务或停机时通知工作线程。
    std::queue<std::function<void()>> jobs_;
    bool stopping_ = false;
    std::vector<std::thread> workers_; // 析构时逐条 join。

    void run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                changed_.wait(lock, [&] { return stopping_ || !jobs_.empty(); });
                if (stopping_ && jobs_.empty()) return; // 已关且任务做完才退出。
                job = std::move(jobs_.front()); // 取得队头的所有权。
                jobs_.pop();                    // 从等待队列移除。
            } // 先放开队列锁，再执行任务，避免其它工作线程都被堵住。
            job(); // 本教学版任务不得抛异常，否则工作线程会终止程序。
        }
    }

public:
    explicit ThreadPool(std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) workers_.emplace_back([this] { run(); });
    }
    ThreadPool(const ThreadPool&) = delete; // 线程和互斥锁不能安全地按值复制。
    ThreadPool& operator=(const ThreadPool&) = delete;
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true; // 以后不接受新任务，已排队任务仍会做完。
        }
        changed_.notify_all(); // 空队列上的工作线程也要醒来检查停机。
        for (auto& worker : workers_) worker.join(); // 等所有任务和线程结束。
    }
    void submit(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) throw std::runtime_error("pool is stopping");
            jobs_.push(std::move(job));
        }
        changed_.notify_one(); // 一条新任务通常只需唤醒一个工作线程。
    }
};

int main() {
    std::mutex result_mutex; // 两个任务修改同一个计数时需要保护。
    int completed = 0;
    {
        ThreadPool pool(2); // 提前创建两条工作线程。
        for (int i = 0; i < 3; ++i) {
            pool.submit([&] {
                std::lock_guard<std::mutex> lock(result_mutex);
                ++completed;
            });
        }
    } // 析构 pool：先做完队列中的三条任务，再 join 工作者。
    assert(completed == 3);
    std::cout << "completed tasks: " << completed << '\n';
}
```
