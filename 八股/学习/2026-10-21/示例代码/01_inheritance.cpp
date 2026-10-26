// C++17：public 继承、组合、访问权限与构造次序。
// <cassert> 提供 assert；条件为假时终止程序，便于核对观察结果。
#include <cassert>
// <iostream> 提供 std::cout，用于显示构造日志与最后的检查结果。
#include <iostream>

// 只给构造过程记序号；本例是单线程，不涉及并发共享。
int construction_step = 0;

class Instrument {
public:
    // explicit 防止一个 int 在普通隐式转换中变成 Instrument。
    explicit Instrument(int id) : id_(id) {
        ++construction_step;
        assert(construction_step == 1);
        std::cout << "1 Instrument base\n";
    }

    // 函数末尾的 const 表示通过该成员函数不修改普通成员状态。
    int id() const { return id_; }

protected:
    // 派生类可在符合 protected 访问规则的上下文使用这个函数。
    int lot_size() const { return 100; }

private:
    // id_ 存在于基类子对象中，派生类不能直接按名字访问它。
    int id_;
};

class OrderCounter {
public:
    // 每个 OrderCounter 对象记录自己被构造时的序号。
    OrderCounter() : step_(++construction_step) {
        std::cout << step_ << " OrderCounter member\n";
    }

    int step() const { return step_; }

private:
    int step_;
};

// Stock 是一种 Instrument：用 public 明确表达公开的基类关系。
class Stock : public Instrument {
public:
    // 先构造 Instrument 基类，再按成员声明顺序构造两只计数器。
    explicit Stock(int id)
        : Instrument(id), orders_(), cancels_() {
        ++construction_step;
        assert(construction_step == 4);
        std::cout << "4 Stock constructor body\n";
    }

    int minimum_order() const { return lot_size(); }
    int first_member_step() const { return orders_.step(); }
    int second_member_step() const { return cancels_.step(); }

    // 错误片段只留注释：int bad() const { return id_; }
    // private 没有变为派生类可见，应使用公开的 id()。

private:
    // 这两行声明的先后次序决定成员初始化次序。
    OrderCounter orders_;
    OrderCounter cancels_;
};

class Logger {
public:
    // 普通成员函数；Logger 只负责数一数发生了几次记录。
    void record() { ++count_; }
    int count() const { return count_; }

private:
    int count_ = 0;
};

class Gateway {
public:
    // 委托：Gateway 收到请求后调用自己拥有的 Logger 成员。
    void send() { logger_.record(); }
    int sent() const { return logger_.count(); }

private:
    // 组合：Gateway 有一个 Logger，但 Gateway 不是一种 Logger。
    Logger logger_;
};

// const Instrument& 是只读借用，不复制 Stock，也不接管销毁责任。
int read_id(const Instrument& instrument) {
    return instrument.id();
}

int main() {
    Stock stock{600000};
    assert(construction_step == 4);
    assert(stock.first_member_step() == 2);
    assert(stock.second_member_step() == 3);
    assert(stock.minimum_order() == 100);

    // public 且无歧义的继承允许这里的派生类到基类引用转换。
    const Instrument& base = stock;
    assert(base.id() == 600000);
    assert(read_id(stock) == 600000);

    Gateway gateway;
    gateway.send();
    gateway.send();
    assert(gateway.sent() == 2);
    // 编译错误示意：Logger& wrong = gateway; // 组合不提供此转换。

    std::cout << "inheritance and composition checks passed\n";
}
