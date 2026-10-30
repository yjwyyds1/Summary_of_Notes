// ============================================================================
// 01_lambda_function.cpp
// 目的：看清 lambda 的“闭包对象”本质，以及 std::function 的类型擦除成本。
// 前置：知道 lambda 基本写法、std::function 能存可调用对象（01、02 号专题）。
// 程序步骤：
//   1. 比较无捕获 / 小捕获 / 大捕获 lambda 的 sizeof；
//   2. 比较 std::function 的 sizeof，与 lambda 自身大小无关；
//   3. 用 shared_ptr 安全地延长捕获对象的寿命；
//   4. 演示空 std::function 调用抛 std::bad_function_call。
// 观察点：
//   - 有捕获的 lambda 是一个有成员的对象，大小随捕获变化；
//   - std::function 大小固定，内部做小对象优化或堆分配；
//   - 按引用捕获 + 对象先销毁 = 悬垂（本例只演示安全写法，不运行未定义行为）。
// 编译：g++ -std=c++17 -O0 -Wall -Wextra 01_lambda_function.cpp -o 01_lambda_function
// ============================================================================

#include <functional>  // std::function、std::bad_function_call
#include <iostream>    // std::cout
#include <memory>      // std::shared_ptr、std::make_shared
#include <string>      // std::string

int main() {
    // ------------------------------------------------------------------
    // 1. 无捕获的 lambda
    // ------------------------------------------------------------------
    auto no_capture = [](int value) { return value * 2; };
    std::cout << "sizeof(无捕获 lambda)   = " << sizeof(no_capture) << "\n";

    // 无捕获 lambda 可以隐式转成函数指针
    int (*function_pointer)(int) = no_capture;
    std::cout << "转成函数指针后调用: " << function_pointer(21) << "\n";

    // ------------------------------------------------------------------
    // 2. 小捕获与大捕获
    // ------------------------------------------------------------------
    int small_capture_value = 10;
    auto small_capture = [small_capture_value](int value) { return value + small_capture_value; };
    std::cout << "sizeof(捕获一个 int)    = " << sizeof(small_capture) << "\n";

    std::string large_capture_text(64, 'x');   // 64 字节字符串
    auto large_capture = [large_capture_text](int value) {
        return value + static_cast<int>(large_capture_text.size());
    };
    std::cout << "sizeof(捕获一个长 string) = " << sizeof(large_capture) << "\n";

    // ------------------------------------------------------------------
    // 3. std::function：大小固定，与内部可调用对象无关
    // ------------------------------------------------------------------
    std::function<int(int)> boxed_small = small_capture;
    std::function<int(int)> boxed_large = large_capture;
    std::cout << "\nsizeof(std::function<int(int)>) = " << sizeof(std::function<int(int)>) << "\n";
    std::cout << "装小 lambda 后大小不变: " << sizeof(boxed_small) << "\n";
    std::cout << "装大 lambda 后大小不变: " << sizeof(boxed_large) << "\n";
    std::cout << "调用结果：" << boxed_small(1) << " / " << boxed_large(1) << "\n";

    // ------------------------------------------------------------------
    // 4. 用 shared_ptr 安全地延长生命周期
    // ------------------------------------------------------------------
    std::function<void()> callback;
    {
        auto session = std::make_shared<std::string>("session-42");
        // 按值捕获 shared_ptr：引用计数 +1，闭包里持有对象
        callback = [session] { std::cout << "回调里使用: " << *session << "\n"; };
    }   // 作用域的 session 销毁，但闭包里的 shared_ptr 仍让对象存活
    callback();   // 安全：对象由 shared_ptr 续命

    // ------------------------------------------------------------------
    // 5. 空 std::function 调用
    // ------------------------------------------------------------------
    std::function<void()> empty;
    std::cout << "\n空 std::function 是否为真: " << (empty ? "是" : "否") << "\n";
    try {
        empty();   // 抛 std::bad_function_call，而不是段错误
    } catch (const std::bad_function_call&) {
        std::cout << "调用空 std::function：捕获到 std::bad_function_call\n";
    }

    std::cout << "\n提示：把 [session] 换成 [&session] 再让作用域结束，就是悬垂引用——本例不运行这种写法\n";
    return 0;
}
