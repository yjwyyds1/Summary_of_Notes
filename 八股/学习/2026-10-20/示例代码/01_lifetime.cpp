// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 目标：分开看“存储还能放东西”与“某个对象现在能否被使用”。
// A/B：普通局部对象随作用域结束；vector 管理者还管理另一块元素存储。
// 中间：unique_ptr 结束动态对象后，观察指针不会自动改成空指针。
// C：在同一块对齐的字节存储中先后构造两个对象，每次都用 new 返回的新指针。
// D：函数内 static 局部变量跨调用保留值；outer 到 main 末尾才析构。
// 本例只访问活着的对象，不解引用对象析构后留下的旧指针。

#include <cassert>  // assert 检查容器大小、智能指针是否为空；成功时不打印。
#include <cstdio>   // printf/puts 输出对象名和执行阶段；%s/%d 对应字符串/整数。
#include <memory>   // unique_ptr、make_unique：用一个拥有者自动管理动态对象。
#include <new>      // placement new：在给定存储位置构造对象，不在这里另行分配该存储。
#include <vector>   // vector：局部管理对象与其元素存储并不是同一个概念。

// class 的成员默认私有；public 之后的构造、析构和 name 可由外部调用。
class Trace {
public:
    // const char* 是“指向只读字符的指针”，这里接收字符串字面量的首字符位置。
    // 只复制指针值，不复制文字；本例的字面量存活到程序结束，因此 name_ 不会悬垂。
    // explicit 阻止把字符串指针隐式转换成 Trace；noexcept 声明本函数不会抛出异常。
    // : name_(name) 在进入函数体前初始化成员，而不是在函数体里再赋值。
    explicit Trace(const char* name) noexcept : name_(name) {
        std::printf("construct %s\n", name_);
    }

    // 对象生命周期结束时打印；noexcept 表明析构不让异常向外传播。
    ~Trace() noexcept {
        std::printf("destroy %s\n", name_);
    }

    // 前面的 const char* 限制返回指针所指的字符；函数末尾的 const 则修饰成员函数：
    // 本函数不通过 this 修改 Trace 的普通成员，所以 const Trace 也能调用它。
    // 返回的是借来的字符指针，不转交任何释放责任。
    const char* name() const noexcept {
        return name_;
    }

private:
    // 下划线只是成员命名习惯。这里不拥有字符串，因此析构时不 delete name_。
    const char* name_;
};

// 返回 int 的无参函数；static 修饰局部变量，使它不会每次调用都重新从 0 开始。
int next_call() {
    static int calls = 0;
    // 先把保留的 calls 加 1，再返回新值；本例在同一线程里依次调用两次。
    return ++calls;
}

int main() {
    std::puts("A: ordinary local objects");
    // outer 是 main 内普通局部对象，当前构造，直到 main 离开作用域才析构。
    Trace outer("outer");
    {
        // inner 的作用域更短；离开这个块时析构，outer 还在外层继续活着。
        Trace inner("inner");
        // <> 是模板实参：把 vector 的元素类型指定为 int；{} 创建三个现有元素。
        // values 本身在本作用域内，元素存储由它管理，不因“局部变量”就要求元素在栈中。
        std::vector<int> values{10, 20, 30};
        assert(values.size() == 3);
        // 下标从 0 开始；这里 size 为 3，因此读取第一个元素是有效的，输出 10。
        std::printf("first value = %d\n", values[0]);
        // 按构造的逆序销毁 values、inner；vector 清理自身的元素和存储，但不打印。
    }
    std::puts("B: inner block has ended");

    // auto 让编译器从右侧推导类型，此处为 std::unique_ptr<Trace>，不是 Trace 本身。
    // make_unique 分配并构造 Trace，owner 独占它；owner 是局部管理者，被管理对象是动态对象。
    auto owner = std::make_unique<Trace>("dynamic");
    // get() 只借出裸指针，不转移所有权；owner 仍负责最后销毁对象。
    Trace* observer = owner.get();
    // -> 通过有效指针调用成员函数；name() 返回的文字用于 %s 输出。
    std::printf("observe %s\n", observer->name());
    // reset() 立即销毁被拥有的 Trace 并让 owner 变空；不是等到 main 末尾才销毁。
    owner.reset();
    // 原 observer 此时已不能再观察那个对象。手动置空，清楚表示不再使用它。
    // 置空只是改指针变量，不是“再次销毁”对象，也不会修复其他地方的旧指针。
    observer = nullptr;
    // unique_ptr 的布尔判断反映是否拥有对象，!owner 在空指针时为 true。
    assert(!owner);

    std::puts("C: storage and object are separate");
    // sizeof(Trace) 给出放下一个 Trace 所需的字节数；unsigned char 表示原始字节。
    // alignas(Trace) 保证起始地址满足 Trace 的对齐要求。这里只准备存储，还没构造 Trace。
    alignas(Trace) unsigned char storage[sizeof(Trace)];
    // new (storage) 是 placement new：在指定位置调用构造函数，返回新对象的 Trace*。
    // 与普通 new 的差别在于这块存储由外层字节数组提供，不能交给普通 delete。
    Trace* placed = new (storage) Trace("first in-place");
    std::printf("observe %s\n", placed->name());
    // 显式析构只结束这里的 Trace 生命周期；底层 storage 数组仍在 main 的作用域中。
    // 这种手动管理是本段的专门演示；普通局部对象和 unique_ptr 不应照着重复析构。
    placed->~Trace();
    placed = nullptr;

    // 同一存储可以再次构造对象；使用这次 new 返回的 replacement，不沿用旧对象指针。
    Trace* replacement = new (storage) Trace("second in-place");
    std::printf("observe %s\n", replacement->name());
    replacement->~Trace();
    replacement = nullptr;

    // 相同函数被调用两次，static calls 依次返回 1、2，而不是两次都返回 1。
    std::printf("calls: %d\n", next_call());
    std::printf("calls: %d\n", next_call());
    // 这行之后才离开 main：outer 析构，输出 destroy outer。
    std::puts("D: main is about to return");
}

// 观察顺序：construct outer -> construct inner -> first value = 10 -> destroy inner；
// B 后 dynamic 被构造、观察、reset 销毁；C 中两个 in-place 对象各自构造/观察/销毁；
// 最后 calls: 1、calls: 2、D 提示、destroy outer。
// 小规模分配假定成功；分配失败可能抛异常，本例不演示恢复该失败。
