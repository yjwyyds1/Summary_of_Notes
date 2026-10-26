// 学习日期：2026-10-20；注释补充：2026-10-21；基线：C++17。
// 主线：size 表示现有元素数，capacity 表示不重新分配就能容纳的元素数。
// 1. reserve 只请求容量；resize 才按需要增减现有元素。
// 2. 重新分配会使 vector 元素的旧指针/引用/迭代器失效，旧地址不会自动跟随新元素。
// 3. 保存索引后重新取元素，或把独立对象交给 unique_ptr 管理，解决的是两种不同需求。
// 4. erase 后接住返回的迭代器；用 push_back/emplace_back 表达不同的构造参数形式。
// 所有访问都在有效对象和合法范围内；错误访问只在专题中静态分析，不在本程序执行。
// 正常输出 vector safe examples: ok；断言成功时不另行打印。

#include <cassert>    // assert 检查教学步骤的性质；NDEBUG 可关闭它，不能靠它执行业务操作。
#include <cstddef>    // std::size_t：表达容器下标/大小的无符号整数类型。
#include <iostream>   // cout 输出最后的正常完成标记。
#include <memory>     // unique_ptr/make_unique：把独立 int 的所有权与 vector 槽位分开。
#include <stdexcept>  // out_of_range：捕获 at() 在越界时规定抛出的异常。
#include <string>     // string：用它展示复制现成对象与按参数原地构造的区别。
#include <vector>     // vector：管理一段连续的元素序列以及其容量。

// void 函数不返回结果；这里先演示容量、元素个数和有效观察指针的关系。
void basic_operations() {
    // <> 指定模板元素类型为 int；默认构造时没有元素，size 为 0。
    std::vector<int> values;
    // 请求至少容纳 4 个元素的空间；不创建那 4 个 int，size 仍是 0。
    values.reserve(4);
    assert(values.empty());
    // >= 而不是 ==：标准保证至少满足请求，不要求容量恰好等于 4。
    assert(values.capacity() >= 4);

    // bool 只记录“是否捕获了预期异常”，初始 false。
    bool caught = false;
    try {
        // at(0) 检查范围；此时 size 为 0，所以会抛出 out_of_range。
        // (void) 表示忽略返回值。这是规定的异常路径，不是执行越界 [] 的 UB。
        (void)values.at(0);
    } catch (const std::out_of_range&) {
        // const & 借用异常对象；本例不需要读取它，因此参数没有写名字。
        caught = true;
    }
    assert(caught);

    // resize(2) 真正创建两个 int 元素，此处值初始化为 0；现在才可访问下标 0、1。
    values.resize(2);
    assert(values.size() == 2);
    // && 是逻辑“并且”，左右两个条件都为真，整条断言才成立。
    assert(values[0] == 0 && values[1] == 0);
    values[0] = 100;
    values[1] = 101;
    // 声明里的 * 表示指针类型；&values[0] 在表达式中取得首元素的当前地址。
    // first 只是观察，不拥有该 int，也不能自己 delete 它。
    int* first = &values[0];
    values.push_back(102); // 新 size 为 3，不超过 capacity，因此本次不重新分配。
    // 原元素指针仍有效；这里没有保存或使用已经失效的旧 end 迭代器。
    assert(*first == 100);

    int sum = 0;
    // begin() 给出首元素迭代器，end() 是尾后位置，不指向可读取的元素。
    // auto 推导迭代器的具体类型，不要求它一定是裸指针；++it 移向下一元素。
    // 条件确保 it != end 才进入循环，因而循环体内的 *it 可以读取当前元素。
    for (auto it = values.begin(); it != values.end(); ++it) {
        sum += *it;
    }
    assert(sum == 303);
    // 缩小到 1：保留首元素，销毁后两个；不是清空再新建首元素。
    values.resize(1);
    assert(values.size() == 1);
    assert(*first == 100); // 首元素仍被保留，所以 first 还可读；函数退出后就不能再用。
}

// 方法一：保存位置编号，而不保存 vector 旧元素的地址。
void fix_with_index() {
    std::vector<int> prices;
    prices.push_back(100);
    // const 表示不改这个下标变量；size_t 与 size/capacity 的类型配合。
    const std::size_t best_index = 0;
    // const auto 让编译器推导容量的返回类型，同时保存不可修改的旧容量副本。
    const auto old_capacity = prices.capacity();
    // 请求值大于当前容量，因此成功时必然重新分配；不再使用任何旧元素地址。
    // 本例容量很小，假定 +1 可表示、请求合法且分配成功，不是任意大容量的恢复示例。
    prices.reserve(old_capacity + 1);
    assert(prices.capacity() > old_capacity);
    // 有效下标必须小于 size；“索引是整数没悬垂”不代表可以越界。
    assert(best_index < prices.size());
    assert(prices[best_index] == 100); // 从当前容器取当前位置，不沿着旧地址找。

    // 但下标只追踪位置，不自动追踪业务身份。插入/删除/排序可能改变位置与值的对应。
    std::vector<int> shifted{100, 200};
    const std::size_t old_index = 1;
    // 在 begin() 指向的位置之前插入 50，序列变成 [50, 100, 200]。
    shifted.insert(shifted.begin(), 50);
    assert(shifted[old_index] == 100); // 下标 1 仍合法，但已经不是原先要找的 200。
    assert(shifted[2] == 200);
}

// 方法二：vector 存管理者，每个 int 独立存在。需要跨管理者搬迁保留的是 int 的地址。
void fix_with_owned_object() {
    // 嵌套 <>：元素类型是 unique_ptr<int>，不是 int；每个管理者独占一个动态 int。
    std::vector<std::unique_ptr<int>> prices;
    // make_unique 返回临时拥有者；push_back 用移动把这份所有权交给 vector 的元素。
    prices.push_back(std::make_unique<int>(100));
    // get() 只借出被拥有 int 的指针，不接管所有权；它不是 &prices[0] 那个管理者地址。
    int* best = prices[0].get();
    const auto old_capacity = prices.capacity();
    // 这里搬迁 vector 中的 unique_ptr 管理者，独立分配的 int 不因此换地址。
    prices.reserve(old_capacity + 1);
    assert(prices.capacity() > old_capacity);
    // 比较的两边都指向仍活着的独立 int，不是在比较已失效的 vector 旧元素指针。
    assert(best == prices[0].get());
    assert(*best == 100);
    // prices 析构时销毁 unique_ptr，进而删除 int；best 不会延寿，之后不能再访问。
    // 若提前 reset/erase/clear 删除这个拥有者，对 best 也要重新判断有效期。
}

// 安全删除与构造：观察迭代器由谁更新，以及追加接口接收的是什么。
void erase_and_construct() {
    std::vector<int> numbers{1, 2, 3, 4};
    // for 的第三部分故意留空：删除与保留两条路径分别决定怎样更新 it。
    for (auto it = numbers.begin(); it != numbers.end();) {
        // *it 读当前元素，% 是取余；除以 2 余数为 0 表示偶数。
        if (*it % 2 == 0) {
            // erase 使旧 it 失效，返回接下来的有效位置（可能是 end），立即接住它。
            // 这里不能再额外 ++it，否则会漏掉接上来的元素，甚至错误越过 end。
            it = numbers.erase(it);
        } else {
            // 没删除时原 it 有效，才正常前进一步。
            ++it;
        }
    }
    // 额外一层括号让 assert 宏把初始化列表里的逗号留在同一个表达式内。
    assert((numbers == std::vector<int>{1, 3}));

    std::vector<std::string> names;
    names.reserve(2);
    const std::string name = "ABC";
    names.push_back(name);       // 接收已有的 const string 左值，在尾部复制出新 string。
    names.emplace_back(3, 'x');  // 把“3 个字符 x”作为构造参数，在尾部直接构造 xxx。
    // 不是“emplace 总更快”：如果给它已有的 name，也仍可能复制，扩容成本也不会消失。
    assert(names[0] == "ABC");
    assert(names[1] == "xxx");
}

int main() {
    // 按这四个独立小实验顺序阅读，每个函数结束会自动清理自己的容器和拥有对象。
    basic_operations();
    fix_with_index();
    fix_with_owned_object();
    erase_and_construct();
    // 这行说明程序走到了末尾；若断言启用，前面的检查也没有使它终止。
    // 一次输出不等于对所有改写版本的无 UB 或性能证明。
    std::cout << "vector safe examples: ok\n";
}
