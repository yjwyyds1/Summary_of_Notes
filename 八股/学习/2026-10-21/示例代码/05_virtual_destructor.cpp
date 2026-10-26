// C++17：虚析构保证 unique_ptr<Channel> 能完整销毁真实派生对象。
// 所有可执行路径都安全；非虚析构的错误设计仅在配套 Markdown 中静态分析。

#include <cassert>   // 提供 assert：核对析构前后的计数。
#include <iostream>  // 提供 std::cout：观察构造、业务与析构顺序。
#include <memory>    // 提供 std::unique_ptr 和 std::make_unique。
#include <string>    // 提供 std::string：保存通道名称。

// 一个小计数器：在拥有者销毁后仍能检查两个析构函数是否执行。
struct LifetimeCount {
    int derived = 0;  // 派生类析构函数体执行次数，起始为 0。
    int base = 0;     // 基类析构函数体执行次数，起始为 0。
};

// Channel 是抽象通信接口：调用者可使用，也可通过基类拥有并销毁它。
class Channel {
public:
    // 接收一个已存在计数器的引用；本类不拥有该计数器。
    explicit Channel(LifetimeCount& counts) : counts_(counts) {
        std::cout << "Channel ctor\n";
    }

    // public 允许外部删除；virtual 使删除遵循真实派生类型。
    // 析构函数不能写返回类型；它不是普通的 void 成员函数。
    virtual ~Channel() {
        ++counts_.base;  // ++ 把计数加一，记录基类析构函数体执行。
        std::cout << "Channel dtor\n";
    }

    // = 0 让 Channel 保持抽象；const 表示调用不修改普通成员。
    virtual int send() const = 0;

private:
    LifetimeCount& counts_;  // 非拥有引用；main 保证计数器活得更久。
};

// FileChannel 是一个具体通道；final 表示不再继续派生。
class FileChannel final : public Channel {
public:
    // 两个形参分别提供计数器和名称；名称以 const 引用避免形参复制。
    FileChannel(LifetimeCount& counts, const std::string& name)
        : Channel(counts), name_(name), counts_(counts) {
        // 先构造基类，再按声明顺序初始化 name_、counts_，最后进入此处。
        std::cout << "FileChannel ctor\n";
    }

    // 基类析构已经 virtual，因此派生析构可用 override 核对覆盖关系。
    ~FileChannel() override {
        ++counts_.derived;  // 只记录派生析构函数体，不手工调用基类析构。
        std::cout << "FileChannel dtor\n";
        // 函数体结束后自动销毁本类成员，再自动销毁 Channel 基类子对象。
    }

    // override 核对 send 的返回类型、参数和 const 都与基类要求兼容。
    int send() const override {
        std::cout << "send: " << name_ << '\n';
        return 7;  // 教学用固定返回值，不进行真实文件或网络操作。
    }

private:
    std::string name_;       // 拥有名称文本；析构会自动清理其内部资源。
    LifetimeCount& counts_;  // 借用同一个计数器，供派生析构记录。
};

int main() {  // 程序入口；以下花括号用来明确拥有者的销毁时刻。
    LifetimeCount counts;  // 在内层作用域之前创建，在其之后才销毁。

    {
        // make_unique<FileChannel> 创建真实 FileChannel。
        // 转为 unique_ptr<Channel> 后仍是同一对象，并未复制成 Channel。
        std::unique_ptr<Channel> channel =
            std::make_unique<FileChannel>(counts, "quotes");

        // -> 通过智能指针访问对象；虚调用进入 FileChannel::send。
        const int result = channel->send();
        assert(result == 7);
        assert(counts.derived == 0 && counts.base == 0);

        std::cout << "before owner leaves\n";
    }  // channel 析构：默认删除器经 Channel* 删除，虚析构选择完整链条。

    // 内层拥有者已经退出，外层计数器还活着，可以安全读取。
    assert(counts.derived == 1);
    assert(counts.base == 1);
    std::cout << "after owner leaves\n";
    return 0;
}
