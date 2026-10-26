// ============================================================
// 第17章 数组 完整示例 + 超详细中文注释（C++17 兼容版）
// 编译：g++ -std=c++17 only_one.cpp -o only_one
// ============================================================

#include <array>          // std::array
#include <vector>         // std::vector（对比用）
#include <iostream>
#include <string>
#include <string_view>
#include <cstddef>        // std::size_t
#include <cassert>        // assert
#include <algorithm>      // std::shuffle
#include <random>         // 随机数
#include <functional>     // std::reference_wrapper
#include <limits>         // 输入清洗用
#include <iterator>       // std::size（C++17）

// ============================================================
// 0. 简单随机数工具（替代教程里的 Random.h）
// 设计意图：把随机逻辑集中，方便全局共用同一个引擎
// ============================================================
namespace Random {
    // 使用 Mersenne Twister，质量高、周期长
    inline std::mt19937 mt{ std::random_device{}() };

    // 返回 [min, max] 闭区间的随机整数
    template <typename T>
    T get(T min, T max) {
        return std::uniform_int_distribution<T>{min, max}(mt);
    }
}

// ============================================================
// 辅助：安全的有符号长度（替代 C++20 的 std::ssize）
// 设计意图：避免有符号/无符号比较警告，也避免 size()-1 下溢
// ============================================================
template <typename T>
constexpr auto ssize(const T& c) {
    return static_cast<std::ptrdiff_t>(std::size(c));
}

// ============================================================
// 1. 固定大小数组 vs 动态数组 —— 核心对比
// ============================================================
/*
关键认知：
- std::array<int,5> 和 std::array<int,10> 是【两个完全不同的类型】
- std::vector<int> 不管存多少个元素，都是同一个类型
- 这直接决定了：传参要不要写模板、长度能不能运行时决定
*/
void demo_fixed_vs_dynamic() {
    std::cout << "\n===== 1. 固定大小 vs 动态数组 =====\n";

    // ---------- 固定大小：std::array ----------
    // 长度是类型的一部分（非类型模板参数），必须编译期知道
    constexpr std::array<int, 4> fixed{ 10, 20, 30, 40 };
    // 好处1：可以做成 constexpr，零运行时开销，还能参与其他常量表达式
    // 好处2：几乎零额外开销（内部就是原生数组）
    // 坏处：长度写死，不能改

    // ---------- 动态大小：std::vector ----------
    std::size_t n = 5;               // 运行时才知道的长度
    std::vector<int> dynamic(n);     // 完全合法
    dynamic.push_back(99);           // 还能继续扩容
    // 好处：灵活
    // 坏处：有容量管理开销，数据在堆上

    // 对比：下面这行如果取消注释会编译失败
    // std::array<int, n> error;     // n 不是常量表达式
}

// ============================================================
// 2. std::array 核心用法与设计意图
// ============================================================
void demo_std_array() {
    std::cout << "\n===== 2. std::array 详解 =====\n";

    // 2.1 长度必须是常量表达式（类型的一部分）
    constexpr int LEN = 4;
    std::array<int, LEN> a{ 1, 2, 3, 4 };   // OK

    // 2.2 聚合初始化（没有用户提供的构造函数）
    // 为什么是聚合？为了零开销，直接初始化内部的原生数组
    std::array<int, 3> b{ 10, 20, 30 };     // 聚合初始化，不是调用构造函数
    // 对比 vector：
    // std::vector<int> v(3, 0);            // vector 有构造函数
    // std::array<int,3> c(3, 0);           // 错误！array 没有这种构造函数

    // 2.3 CTAD（C++17）：让编译器推导类型和长度
    // 好处：永远不会出现“写了4个值却声明成3个”的错误
    std::array arr{ 1, 2, 3, 4 };           // 推导出 std::array<int,4>
    // 注意：元素类型必须一致
    // std::array mixed{ 1, 2.5 };          // 编译错误

    // 2.4 获取长度的方式
    std::cout << "size()     = " << arr.size() << '\n';       // 无符号
    std::cout << "std::size  = " << std::size(arr) << '\n';   // 同上（C++17）
    std::cout << "ssize()    = " << ssize(arr) << '\n';       // 我们自己写的有符号版本

    // 为什么推荐有符号长度？避免有符号/无符号比较警告，也避免 size()-1 下溢
    for (int i = 0; i < ssize(arr); ++i) {   // 干净，无警告
        std::cout << arr[i] << ' ';
    }
    std::cout << '\n';

    // 2.5 三种索引方式对比
    // arr[i]          ：最快，无检查，越界 = 未定义行为（日常用，自己保证合法）
    // arr.at(i)       ：运行时检查，抛异常（一般不推荐，太晚发现错误）
    // std::get<i>(arr)：编译期检查，索引必须是常量（最安全）
    std::cout << "arr[2] = " << arr[2] << '\n';
    std::cout << "std::get<2> = " << std::get<2>(arr) << '\n';
    // std::get<99>(arr);   // 编译直接失败，好！

    // 2.6 传参必须用模板（因为不同长度是不同类型）
    // 设计意图：类型安全 + 零开销
    // 好处：编译器知道确切长度，可以做更多优化
}

// 接受任意长度 std::array 的函数（必须模板化）
template <typename T, std::size_t N>
void print_array(const std::array<T, N>& arr) {   // 按 const 引用传，避免拷贝
    // 为什么用 const 引用？
    // 1. 不拷贝整个数组（大数组时很重要）
    // 2. 保证函数内不会修改原数组
    for (const auto& e : arr) {
        std::cout << e << ' ';
    }
    std::cout << '\n';
}

// ============================================================
// 3. C 风格数组 + 数组衰减（本章最重要概念之一）
// ============================================================
void demo_c_array() {
    std::cout << "\n===== 3. C 风格数组与衰减 =====\n";

    int c_arr[]{ 10, 20, 30, 40 };   // 推荐写法：让编译器数长度

    // 获取长度（C数组没有 .size() 成员）
    std::cout << "std::size(c_arr) = " << std::size(c_arr) << '\n';
    std::cout << "ssize(c_arr)     = " << ssize(c_arr) << '\n';

    // ---------- 数组衰减（Decay）----------
    // 绝大多数情况下，数组名会悄悄变成指向首元素的指针
    int* p = c_arr;                  // 等价于 &c_arr[0]
    std::cout << "sizeof(c_arr) = " << sizeof(c_arr) << '\n';  // 16（4个int）
    std::cout << "sizeof(p)     = " << sizeof(p) << '\n';      // 8（指针大小）
    // 长度信息彻底丢失！这就是衰减的代价

    // 函数参数写成数组形式，实际收到的是指针
    // void foo(int a[]) 等价于 void foo(int* a)
    // 所以在函数内部无法用 range-for，也无法知道长度
}

// ============================================================
// 4. 实战：魔药商店（平行数组 + 枚举哨兵 + static_assert）
// 设计意图：用固定数组实现“查表”，编译期保证数据一致性
// ============================================================
namespace Potion {
    // 枚举 + 哨兵值设计
    // max_potions 既当“元素个数”，又当“无效/退出”信号
    // 好处：一箭双雕，调用方可以用 if (type == max_potions) 判断退出
    enum Type {
        healing,
        mana,
        speed,
        invisibility,
        max_potions          // 哨兵，永远放最后
    };

    // 可迭代的枚举列表（方便 range-for）
    constexpr std::array types{ healing, mana, speed, invisibility };

    // 平行数组：用同一个枚举值当下标，天然对齐
    // 为什么不用 struct？属性少（只有名字和价格）时平行数组更直接
    // 属性超过 2~3 个时，建议改成 struct 数组，避免失同步
    constexpr std::array<std::string_view, max_potions> name{
        "healing", "mana", "speed", "invisibility"
    };
    constexpr std::array cost{ 20, 30, 12, 50 };

    // 三重 static_assert 守门
    // 设计意图：谁改了枚举忘了补数组，编译期立刻爆炸
    // 这比运行时发现错位数据好一万倍
    static_assert(std::size(types) == max_potions);
    static_assert(std::size(name)  == max_potions);
    static_assert(std::size(cost)  == max_potions);
}

class Player {
    static constexpr int s_minStartingGold{ 80 };
    static constexpr int s_maxStartingGold{ 120 };

    std::string m_name{};
    int m_gold{};
    // 库存：用枚举值当下标的计数数组
    // 设计意图：下标即类型，无需 map，O(1) 访问，缓存友好
    std::array<int, Potion::max_potions> m_inventory{};

public:
    explicit Player(std::string_view name)
        : m_name{ name }
        , m_gold{ Random::get(s_minStartingGold, s_maxStartingGold) }
    {}

    // 提供公共访问接口（不要直接访问私有成员）
    const std::string& name() const { return m_name; }
    int gold() const { return m_gold; }
    int inventory(Potion::Type p) const { return m_inventory[p]; }

    // 购买逻辑
    bool buy(Potion::Type type) {
        if (m_gold < Potion::cost[type]) return false;  // 买不起
        m_gold -= Potion::cost[type];
        ++m_inventory[type];
        return true;
    }
};

// 输入清洗示例（最容易写崩的部分）
// 设计意图：同时接受数字和 'q'，并严格过滤脏输入
Potion::Type whichPotion() {
    char input{};
    while (true) {
        std::cin >> input;
        if (!std::cin) {                     // 流坏了（EOF等）
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        // 检测尾部垃圾（比如用户输入了 "2d"）
        if (!std::cin.eof() && std::cin.peek() != '\n') {
            std::cout << "输入有多余字符，请重试: ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (input == 'q') return Potion::max_potions;  // 退出信号

        int val = input - '0';               // 字符转数字（ASCII算术）
        if (val >= 0 && val < Potion::max_potions)
            return static_cast<Potion::Type>(val);

        std::cout << "无效选择，请重试: ";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

// ============================================================
// 5. 实战：扑克牌（查表 + 静态成员 + 游标）
// 设计意图：固定数组 + 游标 = 逻辑大小可变、物理容量不变
// ============================================================
struct Card {
    enum Rank {
        rank_ace, rank_2, rank_3, rank_4, rank_5, rank_6, rank_7,
        rank_8, rank_9, rank_10, rank_jack, rank_queen, rank_king,
        max_ranks
    };
    enum Suit {
        suit_club, suit_diamond, suit_heart, suit_spade,
        max_suits
    };

    // static constexpr：所有 Card 实例共享同一份表，不随对象复制
    // 好处：省内存 + 语义正确（牌面符号是全局的，不是每张牌私有的）
    static constexpr std::array allRanks{
        rank_ace, rank_2, rank_3, rank_4, rank_5, rank_6, rank_7,
        rank_8, rank_9, rank_10, rank_jack, rank_queen, rank_king
    };
    static constexpr std::array allSuits{
        suit_club, suit_diamond, suit_heart, suit_spade
    };

    Rank rank{};
    Suit suit{};

    // 查表打印
    friend std::ostream& operator<<(std::ostream& out, const Card& card) {
        // 局部 static constexpr：只初始化一次，放只读段，作用域更小
        static constexpr std::array ranks{ 'A','2','3','4','5','6','7','8','9','T','J','Q','K' };
        static constexpr std::array suits{ 'C','D','H','S' };
        out << ranks[card.rank] << suits[card.suit];
        return out;
    }

    // 查表算点数（A 先按 11）
    int value() const {
        static constexpr std::array rankValues{ 11,2,3,4,5,6,7,8,9,10,10,10,10 };
        return rankValues[rank];
    }
};

class Deck {
    std::array<Card, 52> m_cards{};
    std::size_t m_nextCardIndex{ 0 };   // 游标：下一张发到哪

public:
    Deck() {
        // 按花色 × 点数铺满 52 张
        std::size_t count = 0;
        for (auto suit : Card::allSuits)
            for (auto rank : Card::allRanks)
                m_cards[count++] = Card{ rank, suit };
    }

    void shuffle() {
        std::shuffle(m_cards.begin(), m_cards.end(), Random::mt);
        m_nextCardIndex = 0;            // 洗牌后必须重置游标，否则牌会“丢”
    }

    Card dealCard() {
        // assert 护栏：牌发完了还调用 = 程序员错误，直接终止并报位置
        // 比返回野值或抛异常更适合“不该发生的情况”
        assert(m_nextCardIndex < 52 && "Deck::dealCard ran out of cards");
        return m_cards[m_nextCardIndex++];  // 返回拷贝，游标前进
    }
};

// ============================================================
// 6. 实战：二十一点核心逻辑（含 A 的 1/11 弹性）
// 设计意图：用辅助计数器弥补“只记总分”带来的信息损失
// ============================================================
namespace Settings {
    constexpr int bust{ 21 };
    constexpr int dealerStopsAt{ 17 };
}

class BlackjackPlayer {
    int m_score{};
    int m_ace11Count{};                 // 手里按 11 算的 A 有几张

public:
    void addToScore(Card card) {
        m_score += card.value();        // A 先按 11 记
        if (card.rank == Card::rank_ace) ++m_ace11Count;
        consumeAces();                  // 立刻做弹性调整
    }

    // 核心技巧：增量记账
    // 一旦总分爆掉，就把 A 逐张从 11 翻转为 1（每张 -10）
    // 直到不爆或 A 用完
    void consumeAces() {
        while (m_score > Settings::bust && m_ace11Count > 0) {
            m_score -= 10;
            --m_ace11Count;
        }
    }

    int score() const { return m_score; }
};

// ============================================================
// 7. std::reference_wrapper：让数组“存引用”
// ============================================================
void demo_reference_wrapper() {
    std::cout << "\n===== 7. std::reference_wrapper =====\n";

    int x = 1, y = 2;
    // C++ 不允许真正的引用数组
    // int& refs[2]{ x, y };             // 编译错误

    // 用 reference_wrapper 曲线救国
    std::array<std::reference_wrapper<int>, 2> refs{ x, y };

    refs[0].get() = 10;                 // 修改 x
    std::cout << "x = " << x << '\n';   // 10

    // operator= 是重新绑定（reseat），不是赋值给被引用对象！
    refs[0] = y;                        // 现在 refs[0] 指向 y
    refs[0].get() = 20;
    std::cout << "y = " << y << '\n';   // 20
    std::cout << "x = " << x << '\n';   // 仍然是 10
}

// ============================================================
// main：把所有演示串起来
// ============================================================
int main() {
    // 演示：constexpr 数组是只读的，不能修改
    // constexpr std::array<int, 4> cost{ 20, 30, 12, 50 };
    // // cost[3] = 0;   // 这行会编译失败，因为 constexpr 对象是 const
    // std::cout << cost[0] << ' ' << cost[1] << ' ' << cost[2] << ' ' << cost[3] << '\n';


    demo_fixed_vs_dynamic();
    demo_std_array();

    std::array demo{ 100, 200, 300 };
    print_array(demo);                  // 模板函数调用

    demo_c_array();
    demo_reference_wrapper();

    // 简单演示牌组
    std::cout << "\n===== 牌组演示 =====\n";
    Deck deck;
    deck.shuffle();
    std::cout << "发三张牌: ";
    for (int i = 0; i < 3; ++i)
        std::cout << deck.dealCard() << ' ';
    std::cout << '\n';

    // 简单演示玩家买药
    std::cout << "\n===== 魔药商店演示 =====\n";
    Player player{ "Hero" };
    std::cout << player.name() << " 初始金币: " << player.gold() << '\n';

    std::cout << "\n程序结束。所有核心概念已覆盖。\n";
    return 0;
}