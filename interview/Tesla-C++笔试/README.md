# Tesla C++ 笔试实操课

你会变量、函数、类和 STL，就从这里开始。整门课用同一个小项目：**main 交几个数字，后台线程一个个取出来处理，最后正常退出。** 今天只做第一关。每关先跑出预期结果，再改一行，最后回答下面的问题。

邮件给出的准备重点是 C++17 工程实现、线程同步、异常与边界。真实题目和通过线未知。考前用第 8 节的闭卷模拟检验自己能不能独立完成一项任务。算法放在后面短练。

## 0. 怎么运行？代码里的名字怎么认？

用 IDE 单独运行每关的 cpp，或者在 PowerShell 执行：

```powershell
Set-Location -LiteralPath 'D:/Summary_of_Notes/interview/Tesla-C++笔试'
g++ -std=c++17 -pthread '.\代码\00_queue_only.cpp' -o "$env:TEMP\tesla-practice.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\tesla-practice.exe" }
```

以后只换上面命令里的文件名。每个 cpp 有自己的 main，单独编译。第一次把它当普通程序运行。 `-std=c++17` 选 C++17，`-pthread` 让编译和链接使用线程支持。编译不通过时先看第一条错误。

就拿你用过的 `std::vector<int> v;` 对照。 `#include <vector>` 让编译器认识这个工具。 `std::` 表示名字来自标准库。 `vector<int>` 是类型，`v` 是自己起的对象名。 `v.push_back(42)` 是对 v 调成员函数。下面的 `std::thread worker(work)`、`std::mutex mutex` 也按这个办法拆。

## 1. 只用 STL，把数放进去再取出来

打开 [00_queue_only.cpp](代码/00_queue_only.cpp)。 `<queue>` 提供 `std::queue<int>`，像排队一样，先放进去的先出来。

```cpp
#include <iostream>
#include <queue>

int main() {
    std::queue<int> numbers;
    numbers.push(0);
    numbers.push(42);
    while (!numbers.empty()) {
        int value = numbers.front();
        numbers.pop();
        std::cout << value << '\n';
    }
}
```

用手记住队列的变化。push(0) 后是 [0]，push(42) 后是 [0,42]。 `empty()` 问是否空；`front()` 读队首；`pop()` 删除队首，返回 void。要取出数字就先保存 front 再 pop，且只能在非空时取。下一次 while 判断又会看当前队列。

**过关**：改成放 10、20、30，预期按同样顺序打印。说出为什么只写 front 不写 pop 会一直打印第一个数。

## 2. 把打印函数交给新线程

打开 [00a_join.cpp](代码/00a_join.cpp)：

```cpp
#include <iostream>
#include <thread>

void work() {
    std::cout << "1. worker is running\n";
}

int main() {
    std::thread worker(work); // 启动线程，在新线程里调用 work。
    worker.join();             // main 等 work 执行完，才继续往下。
    std::cout << "2. main can finish\n";
}
```

`work` 是你写的普通函数。 `work()` 表示当前线程立刻调用；`std::thread worker(work)` 是创建一个名叫 worker 的对象，让新线程调用 work。这个类来自 `<thread>`。从这一行开始，两条执行路线都可能往下走。main 调用 `worker.join()`，意思是“我在这里等那个线程结束”。如果它已经结束，join 就会很快返回。

| 顺序能保证吗 | 原因 |
|---|---|
| work 的输出在 `2. main can finish` 前 | main 在打印前调用了 join |
| main 在 thread 构造后立刻打印的话，一定先于 work 吗 | 两边都能运行，取决于调度 |

**过关**：在 worker.join() 前加一行 `std::cout << "main reached join" << std::endl;`。这行和 work 哪个先出现都可能；join 后那行总在 work 后。每个仍关联着线程的 thread 对象在销毁前要 join。 `joinable()` 是判断还能否 join，已经执行完但还没 join 时也可为真。

## 3. 两个人从同一个队列取数

假设队列只剩一个数。A 发现非空，刚准备读 front；B 把最后一个数取走。A 再去读已空的队列，出错。你要让“检查空、读队首、删队首”三步一起完成。

`<mutex>` 提供 `std::mutex`。每次只有一个线程能持有同一把 mutex。 `mutex.lock()` 拿锁，已经有人持有时会等；`mutex.unlock()` 释放。实际写题用 `std::lock_guard<std::mutex> guard(mutex);`，这是一个局部对象，构造它会拿锁，离开花括号时析构自动解锁。 `<std::mutex>` 和 vector 的 `<int>` 一样是在指定类型。提前 return、抛异常离开这一层，也会析构 guard。

先跑 [00b_mutex.cpp](代码/00b_mutex.cpp)，两个线程各加 2 次，最后 count 是 4。再跑 [00d_try_pop.cpp](代码/00d_try_pop.cpp)，把同一把锁放进你熟悉的 class：

```cpp
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
```

- `Inbox` 是我们自己写的类。它放了一个队列和一把锁。push 与 tryPop 都使用这同一把锁。
- `bool tryPop(int& value)` 用引用把数据写回调用方的 value，返回 true 表示取到了，false 表示现在空。这样 0 也能正常取出。
- `[&] { ... }` 是一个当场定义的小函数，传给 thread 调用。 `[&]` 允许它用外面的变量本身。两个 lambda 使用同一个 inbox，但各改自己的计数。main 等两个线程 join 后才读计数。
- 读线程只是“试一下”。一旦空了就结束，所以这个例子先放 100 个数，再启动线程。

**过关**：把 100 改成 10，预测总数。用手指出拿锁到放锁之间有哪些语句。把填数据的循环挪到两个 join 的后面，读线程会各自发现空队列并退出，预期总数是 0。这正是下一关要解决的事。

## 4. 读的人先来，如何等后来的人写？

先只传一个数。看 [00c_wait.cpp](代码/00c_wait.cpp)。

```cpp
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
```

这里有四个对象或变量：`value` 存数字；`ready` 记录写好没有；`mutex` 保护前两个；`changed` 来自 `<condition_variable>`，供读线程等待、写线程通知。 **changed 里不存 value，也不会自动改变 ready。**

| 假设 reader 先到 | 哪一句做什么 |
|---|---|
| reader 拿锁，发现没数据 | `unique_lock<mutex> lock(mutex)` 构造时拿锁，`while (!ready)` 检查状态 |
| reader 等 | `changed.wait(lock)` 放开 lock 管的那把锁，然后等 |
| main 写 | 拿同一把锁，写 value 和 ready，离开花括号释放锁 |
| main 发通知 | `changed.notify_one()` 通知一个等待者 |
| reader 回来 | wait 先拿回锁才返回。回到 while，再看 ready，已写好就读 |

`unique_lock` 也来自 `<mutex>`。它和 lock_guard 都能在作用域离开时释放锁，但 unique_lock 还允许临时放开再拿回，因此 std::condition_variable 的 wait 要传 **unique_lock 对象**。邮件里的 `wait(mMutex, ...)` 是示意。用另一把锁或把条件的改动移到锁外，会破坏同步。

为什么用 while？线程醒来后，条件可能还不成立，要再问一次。若 main 先完成写入，reader 一开始就看到 ready 为真，直接读。库还提供省略 while 的写法：

```cpp
changed.wait(lock, [&] { return ready; });
```

第二个参数是返回 bool 的小函数，也叫谓词。返回 true 表示可以继续。 **过关**：先把 42 改为 7，再把 while 换成上面的写法。两遍都应输出 reader got 7。说出 wait 期间谁能拿到 mutex。

## 5. 从“等一个数”到“等队列有数或关闭”

把 ready 改成 `!queue_.empty()`，意思是“还有可取的数据”。但什么时候该永远停止等？让 main 用 closed_ 表示以后不再交新数据。wait 的条件是 `closed_ || !queue_.empty()`。 `[this]` 让小函数能读当前的 BlockingQueue 成员。

| 队列里有数吗 | 关闭了吗 | 读线程怎么做 |
|---|---|---|
| 有 | 否或是 | 取数 |
| 没有 | 否 | 放锁并等待 |
| 没有 | 是 | 退出 |

这一版改用 `std::optional<int>` 返回结果。它来自 `<optional>`，好比一个有时空着的盒子。 `return value;` 装一个数，`return std::nullopt;` 给空盒子。调用方先问 `result.has_value()`，确认后才用 `*result` 取数。一个装着 0 的盒子依然有值。

```cpp
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

class BlockingQueue {
public:
    bool push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) {
                return false; // 关门后，不再收新数据。
            }
            queue_.push(value);
        } // 先把数据放好，并释放锁，再通知读线程。
        ready_.notify_one();
        return true;
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 空且没关：等待。否则回来继续判断能不能取。
        ready_.wait(lock, [this] { return closed_ || !queue_.empty(); });

        if (queue_.empty()) {
            return std::nullopt; // 能走到这里但没数据，必定已关。
        }

        int value = queue_.front(); // front 只看队首。
        queue_.pop();               // pop 才真正删除队首。
        return value;               // 包装成有值的 optional<int>。
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true; // 只关门，保留已有数据。
        }
        ready_.notify_all();
    }

private:
    std::mutex mutex_; // 同时保护 queue_ 和 closed_。
    std::condition_variable ready_;
    std::queue<int> queue_;
    bool closed_ = false;
};

int main() {
    BlockingQueue queue;

    std::thread reader([&queue] {
        while (true) {
            std::optional<int> result = queue.pop();
            if (!result.has_value()) {
                std::cout << "reader exits\n";
                break;
            }
            std::cout << "read " << *result << '\n';
        }
    });

    const bool acceptedZero = queue.push(0);
    const bool accepted42 = queue.push(42);
    queue.close();
    reader.join();

    const bool acceptedAfterClose = queue.push(99);
    assert(acceptedZero && accepted42 && !acceptedAfterClose);
    assert(!queue.pop().has_value());
}
```

按三个函数读：

1. **push**。先拿锁看关没关，开着才入队。离开内层花括号先放锁，再叫一个读线程来查。自己写的 `BlockingQueue::push` 管检查和通知，函数体内的 `queue_.push` 是 STL，负责存数。
2. **pop**。先拿锁，在“空且未关”时等。wait 返回可能是因为关闭，也可能是因为有数。仍有数就取队首；没数且关闭就返回 nullopt。
3. **close**。在锁内把 closed_ 变成 true。 `notify_all()` 提醒所有等待者回来检查，让每一条读线程都有机会退出。现存数字此时仍在队列里。main 先 close 再 join，否则读线程等新数，main 等读线程结束。

**过关**：跑 [03_blocking_queue.cpp](代码/03_blocking_queue.cpp)，应先读 0、再读 42、最后退出。再把 reader 的创建移到 close 后、join 前，预期仍一样。最后给自己一张纸，只写出上面三种队列状态、push 唤醒谁、close 唤醒谁。想不起来就回到对应几行看。


## 6. 把课堂队列改成有界队列：先做一个只差一行的练习

“有界”就是队列最多放 N 个数。先试容量为 1：放进 10，再放 20，此时要等有人取走 10。这里增加了一个状态“还有空位吗”。原来的读者等“有数据”，新加的写者等“有空位”。给两种等待各一个 condition_variable。要等待离开时，两边都还要观察 closed_。

下面是在第 5 节基础上的**写者部分**。 `space_` 是新增成员，`capacity_` 是保存上限的整数。这是一段用来理解变化的代码，后面模拟 1 给了完整接口和测试：

```cpp
// std::chrono::milliseconds 来自 <chrono>。这里 time 是最长等待时长。
std::unique_lock<std::mutex> lock(mutex_);
bool canProceed = space_.wait_for(lock, time, [this] {
    return closed_ || queue_.size() < capacity_;
});

if (!canProceed) return PushResult::timeout;
if (closed_) return PushResult::closed;
queue_.push(value);
lock.unlock();
ready_.notify_one();
return PushResult::accepted;
```

`wait_for` 比 wait 多接收一个时间参数。 `std::chrono::milliseconds(5)` 表示 5 毫秒，`5ms` 是启用 `using namespace std::chrono_literals;` 后的写法。wait_for 仍会暂时释放锁。返回 false 表示等待结束时条件仍不成立，true 表示已经满足。满足可能仅仅因为关门了，所以要再看 closed_。 `PushResult` 是题目给的枚举，三种返回值分别是成功、过时、关闭。 `lock.unlock()` 提前放掉锁，之后它析构时不会再放一次。

**现在只填这一行。** 读线程刚执行完 `queue_.pop()` 并放开锁。读者释放了一个空位，要通知谁？先写一句，答案是 `space_.notify_one();`。关门时调用两次 notify_all，让读者和写者都能检查关闭。然后去模拟 1，只做 FIFO 一组测试。通过后再做满、超时和关门。

容易读混的其他名字在 [04_timeout_locks.cpp](代码/04_timeout_locks.cpp)。先只看与本题相近的 wait_for 部分。

| 工具 | 来自 | 读到它时怎么理解 |
|---|---|---|
| timed_mutex 的 try_lock_for | <mutex> | 等的是拿到锁；返回 true 才能访问共享数据 |
| recursive_mutex | <mutex> | 同一个线程能对同一把锁拿多次，拿几次就要放几次 |
| scoped_lock lock(a, b) | <mutex> | 构造时协调拿两把锁，析构时一起释放；C++17 会自己推导类型 |
| defer_lock 与 std::lock | <mutex> | 创建 unique_lock 时先不拿锁，再用 std::lock 一起拿多把 |
| shared_mutex 与 shared_lock | <shared_mutex> | 读者可同时拿共享锁，写者用 unique_lock 独占 |

## 7. 先做会抛错的任务，再取任务结果

### 7.1 异常和锁先分开学

想象你自己写了 `setVolume(int value)`。值必须在 0 到 100。用 `throw std::invalid_argument("volume");` 可以报告非法值。invalid_argument 是 `<stdexcept>` 提供的类，这行构造一个携带说明的异常并抛出。调用方用 try 圈住那次调用，用 `catch (const std::invalid_argument& e)` 接住同类异常。异常往外传时，离开的函数中已构造的局部对象会析构，所以 lock_guard 会释放锁。

先顺着 [05_ownership_exceptions.cpp](代码/05_ownership_exceptions.cpp) 的 main 看三个动作：设置 35 → 尝试设置 -1 → catch 后读回 35。 **动手**：把 -1 改成 101，仍应通过。现在找 configure 中哪一行真正替换配置，想一下为什么非法值发生在那行之前。普通 std::thread 入口不能让异常直接冒出去，否则会终止程序。第 7.3 节会用库把任务异常交给调用方。

### 7.2 指针、回调与模拟 2

用你熟悉的 struct 来看 `Session`。变量若是指针，`p->volume` 等价于 `(*p).volume`；下面这些智能指针也提供箭头。 `<memory>` 提供这三种类型：

- `make_unique<Session>(...)` 创建 Session，返回一个独占拥有者 unique_ptr。拥有者销毁时释放它管理的对象。 `current = std::move(next)` 把资源交给 current。 `move` 来自 `<utility>`，它让这次赋值能接管资源。
- `make_shared<Session>(...)` 创建 Session，返回一个共享拥有者 shared_ptr。复制指针就多一个拥有者。最后一个拥有者消失后，Session 才销毁。 `reset()` 放弃该指针的拥有权。Session 字段若被多线程修改仍需同步。
- `weak_ptr` 只观察对象。 `observer.lock()` 尝试取得 shared_ptr；对象已经销毁就返回空。这里 lock 是尝试取得所有权，与 mutex 是不同类的成员函数。

“回调”就是一段将来由别人调用的函数。比如设备状态改了，由设备调用你注册的函数。 `std::function<void(DeviceState)>` 来自 `<functional>`，能装很多种可调用对象。尖括号里的 `void(DeviceState)` 描述它收一个 DeviceState、不返回结果。 `if (callback)` 问有没有函数，`callback(state)` 调用它。

模拟 2 有这个具体问题：`setVolume` 已拿着设备锁，又在锁内调用 callback。callback 再调用 `snapshot`，后者又想拿同一把锁。用纸画出这三次调用，再去运行 reentry，脚本 8 秒后提示超时。先改成“锁内更新状态、保存快照及函数，离开作用域后再调用”，然后重跑。思考为什么不能解锁后直接读取函数成员：另一个线程可能正在替换它。

### 7.3 主线程怎样拿到工作结果？

第 2 节的 join 只能等工作线程结束。现在 main 要获得一个任务返回的 int，或者知道任务失败了。 `<future>` 提供：

| 名字 | 谁用它 | 做什么 |
|---|---|---|
| `future<int>` | 等结果的一方 | `get()` 等结果，取出 int，任务失败则重抛异常 |
| `promise<int>` | 手动放结果的一方 | `get_future()` 取配对的 future；`set_value(42)` 交付值 |
| `packaged_task<int()>` | 包一个函数 | `get_future()` 取配对的 future；任务执行后自动交付结果或异常 |

`int()` 表示不接收参数、返回 int。 `promise<void>` 只发一个“好了”的信号，所以 set_value 不带参数。 `future.wait()` 只等信号。get 会消费普通 future 的结果，通常只调用一次。哪个线程在执行函数仍由 std::thread 决定。

下面先在当前线程里练一遍，弄懂结果怎样传递：

```cpp
#include <future>
#include <iostream>

int main() {
    std::packaged_task<int()> task([] { return 42; });
    std::future<int> result = task.get_future();
    task();
    std::cout << result.get() << '\n';
}
```

这段应打印 42。第一行包函数，没有执行；第二行拿到结果的读取端；第三行才运行它；最后一行取值。再跑 [07_future.cpp](代码/07_future.cpp)，这次用 std::thread 执行 task，函数故意抛错，看 main 怎样在 result.get() 处接住。模拟 3 的变化是把 task 放到队列里，两个 worker 反复取。**取到任务后要先释放队列锁，再调用 task()**，两个任务才有机会同时运行。

### 7.4 计数和算法只留短练

用两个线程统计完成次数时，可以换成 `<atomic>` 的 `std::atomic_int count{0}`。 `load()` 读，`store(v)` 写；`fetch_add(1)` 一次完成读取、加一和写回，并返回旧值，fetch_sub 类似。load 加 store 分两步，另一个线程能在当中插进来。队列既要判断 closed 又要放数据，仍用一把 mutex。 [06_atomic.cpp](代码/06_atomic.cpp) 里改为每个线程加 3 次，断言最后是 6；`memory_order_relaxed` 用于这个只统计次数的原子变量，初学可先去掉参数用默认值。

[08_algorithm.cpp](代码/08_algorithm.cpp) 用 20 分钟复习就够。 `unordered_set` 记录已经见过的数，insert 返回的 second 表示是否新插入；`lower_bound` 在有序范围找第一个不小于目标的位置；前缀和用 long long 保存，从 {0,10,30,60} 中取下标区间 [1,3) 的和就是 60−10。各补一个空输入或大数字断言。

## 8. 考前怎么判断自己准备到了哪一步？

| 检查 | 做到什么算完成 |
|---|---|
| 跟着跑 | 第 1–5 节的输出和解释能对上。能说清每个对象管什么、谁在调用函数 |
| 看着写 | 给第 5 节的队列加容量。单独通过 FIFO、full、close 三组测试 |
| 独立改 | 模拟 2 修完，能解释重入为什么卡住、在哪里释放锁、异常后状态是什么 |
| 80 分钟闭卷 | 模拟 3 能编译，结果/异常/两个线程并行/关门后排空的用例都通过。记下还要看笔记才能写出的地方，单独重练 |

邮件建议收信后 7 天内完成。开考前用公开练习熟悉 Run Code。正式题按邮件要求独立操作。先分清要改的文件和要求保留的签名，尽早让正常用例运行，最后留约 15 分钟查空输入、超时、异常和关门，再提交。 [Codility 公开练习](https://app.codility.com/demo/take-sample-test/)

本目录的 `.\run.ps1` 会跑全部例程。 `.\run.ps1 -Task 1 -Case fifo` 只跑模拟 1 的 FIFO 测试，失败时看第一条 FAIL。模拟题修改对应的 hpp，测试在同目录的 test.cpp。限时运行期间出现 TIMEOUT 时先查等待条件、遗漏的 notify 和 join 是否持锁。参考实现可以用同样的命令加 `-Reference` 跑，做完再展开答案。



## 9. 三套综合模拟题

在本讲义所在目录运行命令。每题给定公开接口，修改模拟目录中的头文件。测试入口在同目录的 test.cpp。开始时失败属于练习预期，按用例组逐步跑。计时练习先关掉讲义和参考答案，时间到保存当前版本，再对照复盘。三套均为原创练习。

### 模拟 1｜限时有界队列（50 分钟）

**背景**：设备接入线程向后台提交整型消息。队列满时提交者最多等一段时间，服务关闭时须完成已入队的消息。

- `BoundedQueue(capacity)`：容量为 0 抛 `std::invalid_argument`，其余容量不变。
- `pushFor(value, timeout)`：有空位且未关闭就入队并返回 accepted。满且未关闭时等待空位或关闭，超时返回 timeout。观察到关闭时返回 closed，即使还有空位。0ms 立即判断，参数约定非负。timeout 衡量拿到 mutex 后等空位的时长。
- `pop()`：空且未关闭时等。队列非空取队首；关闭且已读空时返回 `nullopt`。
- `close()`：允许重复调用，唤醒所有可能等待的生产者和消费者。三个接口可并发调用。调用方保证所有并发调用结束后才销毁队列。
- 队列空间 O(capacity)，每次入队/出队的容器操作应是 O(1)。任务顺序按实际入队顺序判断。

[待写 queue.hpp](模拟/01-限时有界队列/queue.hpp) · [测试](模拟/01-限时有界队列/test.cpp)

<details>
<summary>展开题目给定的代码</summary>

```cpp
#pragma once
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

enum class PushResult { accepted, timeout, closed };

class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("capacity");
    }

    PushResult pushFor(int value, std::chrono::milliseconds timeout) {
        (void)value;
        (void)timeout;
        // TODO
        return PushResult::timeout;
    }

    std::optional<int> pop() {
        // TODO
        return std::nullopt;
    }

    void close() {
        // TODO
    }

private:
    const std::size_t capacity_;
    // TODO: protected state and wait conditions.
};
```

</details>


```powershell
.\run.ps1 -Task 1 -Case fifo
.\run.ps1 -Task 1 -Case full
.\run.ps1 -Task 1 -Case close
.\run.ps1 -Task 1
```

评分 /100：接口与 FIFO 25，满与限时 25，关闭与排空 30，解释锁与唤醒对象 20。用例没有强制证明等待者已经睡着，自己额外检查“先等后关”和“先关后到”。

### 模拟 2｜修复设备状态回调（35 分钟）

**背景**：同事写了音量状态服务。已有实现能编译，但无效输入会污染状态，某些回调让程序卡住。保留三个接口和返回类型。

- `setVolume(volume)`：0 到 100（含边界）合法。每次合法请求提交新状态并增加 revision，包括音量与旧值相同的情况。无效时返回 false，音量、revision、回调均无变化。假定合法更新总次数不超过 int 上限。
- `snapshot()`：按值返回同一时刻的音量和 revision。
- `setCallback(cb)`：允许注册、替换、注销（空回调）。每次成功提交最多调用一次当时注册的回调，传入提交时的快照。替换或注销不取消已经取得的回调。
- 回调可以调用 snapshot 和 setCallback。回调抛出的异常向调用者传播，已经提交的状态保留。多个调用线程可以并行执行回调，回调实现负责自己的线程安全，执行次序无需按 revision 排序。

[待修 device.hpp](模拟/02-修复设备状态回调/device.hpp) · [测试](模拟/02-修复设备状态回调/test.cpp)

<details>
<summary>展开题目给定的代码</summary>

```cpp
#pragma once
#include <functional>
#include <mutex>
#include <utility>

struct DeviceState {
    int volume = 0;
    int revision = 0;
};

class Device {
public:
    using Callback = std::function<void(DeviceState)>;

    void setCallback(Callback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
    }

    DeviceState snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    // Existing implementation: repair it according to the specification.
    bool setVolume(int volume) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.volume = volume;
        if (volume < 0 || volume > 100) return false;
        if (callback_) callback_(state_);
        ++state_.revision;
        return true;
    }

private:
    mutable std::mutex mutex_;
    DeviceState state_;
    Callback callback_;
};
```

</details>


```powershell
.\run.ps1 -Task 2 -Case state
.\run.ps1 -Task 2 -Case reentry
.\run.ps1 -Task 2 -Case exception
.\run.ps1 -Task 2 -Case concurrent
```

评分 /100：边界、快照与 revision 25，定位并修复回调重入卡死 30，异常提交语义 20，并发与回调生命周期 25。运行 reentry 超时时，画出“持有的锁 → 回调 → 再请求的锁”。建议先修第一个失败用例，再处理超时。

### 模拟 3｜双线程任务执行器（80 分钟）

**背景**：一个服务把耗时命令交给后台执行。调用者要拿到结果，停机时完成已接收任务。可用 C++17 标准库。

- 构造启动两个 worker。提交 `std::function<int()>`，假设调用者提供非空函数且任务最终会返回或抛异常。返回 `future<int>`；get 得到结果或原任务异常。一个任务失败后，后续任务仍可运行。
- 多个生产者可以 submit；提交成功的任务仅执行一次。按 FIFO 取任务，执行完成顺序不作要求。耗时任务在队列锁外执行。任务不要互相等待，也不调用 stop。
- `stop()` 拒绝后续提交（抛 `std::runtime_error`），唤醒 worker，等已接收的任务完成并 join。submit 可以与 stop 并发；受理以锁内入队为准。stop 由一个控制线程串行调用，可以重复。析构等价于停止，调用方保证没有并发 API 调用与析构重叠。
- 加分：处理创建第二个线程失败时的清理。队列允许积压，本题无容量和取消要求。

[待写 executor.hpp](模拟/03-双线程任务执行器/executor.hpp) · [测试](模拟/03-双线程任务执行器/test.cpp)

<details>
<summary>展开题目给定的代码</summary>

```cpp
#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class Executor {
public:
    Executor() {
        // TODO: start exactly two workers.
    }

    ~Executor() {
        stop();
    }

    std::future<int> submit(std::function<int()> function) {
        (void)function;
        // TODO: enqueue, notify, return future.
        return {};
    }

    void stop() {
        // TODO: close, wake and join.
    }

private:
    // TODO: state and worker loop.
};
```

</details>


```powershell
.\run.ps1 -Task 3 -Case result
.\run.ps1 -Task 3 -Case workers
.\run.ps1 -Task 3 -Case stop
.\run.ps1 -Task 3
```

评分 /100：返回结果 20，异常传递且继续工作 20，两任务可并行 20，关闭/积压/析构 30，构造失败清理及解释对象生命周期 10。测试不注入线程创建失败，此项读代码自评。

| 用时 | 目标 |
|---|---|
| 0–10 分钟 | 写下接口契约、共享状态、worker 等待和退出条件 |
| 10–45 分钟 | 做到能提交、取结果并退出，跑 result |
| 45–65 分钟 | 跑 workers 和 stop，修异常、唤醒、join 和关闭竞态 |
| 65–80 分钟 | 全量测试，确认不需要改测试才能通过，复查析构与构造失败 |

完成后记录各项得分、第一次通过编译的时间、每个失败用例的原因。能在 80 分钟内独立完成主路径，并验证异常、并行和排空，再用十分钟复述不变量与关闭顺序，作为备考验收。

## 10. 参考答案与复盘

<details>
<summary>做完题再展开，查看完整实现与解释</summary>

先跑自己的实现，再用 `-Reference` 跑同一组测试。测试对两份实现使用相同的接口和用例。

### 1 限时有界队列

[参考实现 queue.hpp](参考答案/01-限时有界队列/queue.hpp) · ` .\run.ps1 -Task 1 -Reference `

```cpp
#pragma once
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

enum class PushResult { accepted, timeout, closed };

class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("capacity");
    }

    PushResult pushFor(int value, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!space_.wait_for(lock, timeout, [&] {
                return closed_ || queue_.size() < capacity_;
            })) return PushResult::timeout;
        if (closed_) return PushResult::closed;
        try {
            queue_.push(value);
        } catch (...) {
            // The free slot still exists. Give another writer a chance.
            lock.unlock();
            space_.notify_one();
            throw;
        }
        lock.unlock();
        ready_.notify_one();
        return PushResult::accepted;
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return std::nullopt;
        const int value = queue_.front();
        queue_.pop();
        lock.unlock();
        space_.notify_one();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        ready_.notify_all();
        space_.notify_all();
    }

private:
    const std::size_t capacity_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::condition_variable space_;
    std::queue<int> queue_;
    bool closed_ = false;
};
```

画出两个谓词：消费者等 closed 或非空；生产者等 closed 或未满。两个条件变量由同一个 mutex 保护，入队叫醒消费者，出队叫醒生产者，关闭时两边都广播。pushFor 的谓词返回真后必须优先判断 closed。pop 则优先处理已有数据。仔细看入队分配失败时为何再次通知生产者：被叫醒的线程没用掉这个空位，其他写者可能还在等。

复盘：如果你用了一个条件变量和 notify_one，想一想它有没有可能叫醒同类线程，让真正能推进的另一类线程继续等待。加练：两生产者各推 100 个不同 ID，两消费者处理，验证已接收集合与取出集合一致。

### 2 修复设备状态回调

[参考实现 device.hpp](参考答案/02-修复设备状态回调/device.hpp) · ` .\run.ps1 -Task 2 -Reference `

```cpp
#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

struct DeviceState {
    int volume = 0;
    int revision = 0;
};

class Device {
public:
    using Callback = std::function<void(DeviceState)>;

    void setCallback(Callback callback) {
        auto next = callback ? std::make_shared<const Callback>(std::move(callback)) : nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback_.swap(next);
        }
        // A replaced callback and its captures are destroyed outside the lock.
    }

    DeviceState snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    bool setVolume(int volume) {
        if (volume < 0 || volume > 100) return false;
        DeviceState committed;
        std::shared_ptr<const Callback> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.volume = volume;
            ++state_.revision;
            committed = state_;
            callback = callback_;
        }
        if (callback) (*callback)(committed);
        return true;
    }

private:
    mutable std::mutex mutex_;
    DeviceState state_;
    std::shared_ptr<const Callback> callback_;
};
```

先验证音量，锁内修改状态并保存快照和回调的共享所有权，解锁后调用。回调注销自己时，局部 shared_ptr 让正在调用的函数仍然存活。注册时先构造新回调，再锁内交换。被替换的函数及其捕获对象在锁外销毁，避免用户自定义析构函数在锁内运行。回调异常出现在状态提交后，按题意原样传播。

复盘：为什么锁外不能直接使用指向成员 callback_ 的引用？为什么回调收到的 revision 可以比后来 snapshot 查到的旧？如果回调 B 要求严格在回调 A 之后执行，需要新定义交付顺序，可考虑串行派发器，单靠这把状态锁无法实现锁外回调的顺序。

### 3 双线程任务执行器

[参考实现 executor.hpp](参考答案/03-双线程任务执行器/executor.hpp) · ` .\run.ps1 -Task 3 -Reference `

```cpp
#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class Executor {
public:
    Executor() {
        workers_.reserve(2);
        try {
            for (int i = 0; i < 2; ++i)
                workers_.emplace_back([this] { run(); });
        } catch (...) {
            stop();
            throw;
        }
    }

    ~Executor() {
        stop();
    }

    std::future<int> submit(std::function<int()> function) {
        std::packaged_task<int()> task(std::move(function));
        auto result = task.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) throw std::runtime_error("executor stopped");
            tasks_.push(std::move(task));
        }
        ready_.notify_one();
        return result;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        ready_.notify_all();
        for (auto& worker : workers_)
            if (worker.joinable()) worker.join();
    }

private:
    void run() {
        for (;;) {
            std::packaged_task<int()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [&] { return closed_ || !tasks_.empty(); });
                if (tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task(); // The result or task exception is stored in its future.
        }
    }

    std::mutex mutex_;
    std::condition_variable ready_;
    std::queue<std::packaged_task<int()>> tasks_;
    bool closed_ = false;
    std::vector<std::thread> workers_;
};
```

`packaged_task` 可移动，直接存进队列，不需要为它包装 shared_ptr。submit 先取得 future，再锁内检查关闭并把任务移入队列。worker 锁内拿任务，锁外运行，packaged_task 将任务返回值或异常存入 future。stop 锁内设 closed，再广播、锁外 join。worker 只在队列为空且关闭时离开，保证已接收任务处理完。构造捕获线程创建异常，关闭并 join 已启动线程，再抛出异常。

复盘：为什么拿着队列锁做 join 会卡住？如果 stop 允许两个控制线程同时调用，join 需要什么额外协调？如果第二个 worker 创建失败，为什么仅依赖成员默认析构会 terminate？

### 最后五分钟检查表

- 接口签名、包括 0 和空值在内的返回语义、错误码与异常类型符合题意。
- 检查与修改同一业务不变量时持有同一把锁。读路径也受保护。
- 等待都有谓词。每条可能让谓词变真的路径都考虑通知。
- 关闭顺序明确。锁外执行用户代码和 join，所有等待者最终可退出。
- 本地修改后重新编译和 Run Code。覆盖正常、边界、失败和停机，并留出正式提交时间。

</details>
