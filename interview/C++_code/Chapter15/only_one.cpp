// ============================================================
// 第15章 完整演示：this / 方法链 / 嵌套类型 / 静态成员 / 友元 / 类模板
// 修正版：去掉重复定义，保证能直接编译通过
// ============================================================

#include <iostream>
#include <string>
#include <string_view>
#include <random>

// ------------------------------------------------------------
// 1. 类模板 Array —— 演示“类模板的成员函数”写法
// ------------------------------------------------------------
template <typename T>
class Array
{
private:
    T m_data[5]{};

public:
    // ---------- 模板内定义（推荐写法） ----------
    // 好处：直接用 T，编译器看到完整定义，方便内联
    void set(int index, const T& value)
    {
        // this 是隐藏参数，类型 Array<T>* const
        m_data[index] = value;          // 等价于 this->m_data[index] = value;
    }

    T get(int index) const              // const 函数：this 变成 const Array<T>* const
    {
        return m_data[index];
    }

    // 如果想演示“模板外定义”的语法，正确做法是：
    // 1. 在类内只写声明：  void set(int index, const T& value);
    // 2. 在类外写完整定义（见下面注释掉的代码）
    // 但同一个函数不能既在类内定义又在类外定义，否则就重定义错误。
};

/*
// ---------- 模板外定义语法演示（正确写法，但本文件已在类内定义，故注释掉） ----------
// 两个缺一不可：
//   ① template <typename T>
//   ② Array<T>::
template <typename T>
void Array<T>::set(int index, const T& value)
{
    m_data[index] = value;
}

template <typename T>
T Array<T>::get(int index) const
{
    return m_data[index];
}
*/

// ------------------------------------------------------------
// 2. Monster 类
// ------------------------------------------------------------
class Monster
{
public:
    // 嵌套枚举：只服务于 Monster，作用域干净，语义清晰
    enum Type
    {
        dragon, goblin, ogre, orc, skeleton, troll, vampire, zombie,
        maxMonsterTypes                 // 哨兵值，永远放最后
    };

private:
    Type        m_type{};
    std::string m_name{"???"};
    std::string m_roar{"???"};
    int         m_hitPoints{};

    // 静态成员变量：所有对象共享一份，属于“类”而不是某个对象
    static inline int s_alive{0};       // C++17 起可类内直接初始化

public:
    // 构造函数：成员初始化列表 + string_view（避免多余拷贝）
    Monster(Type type, std::string_view name, std::string_view roar, int hitPoints)
        : m_type{type}
        , m_name{name}
        , m_roar{roar}
        , m_hitPoints{hitPoints}
    {
        ++s_alive;
    }

    ~Monster()
    {
        --s_alive;
    }

    // this 解决命名冲突
    void setHitPoints(int hitPoints)
    {
        // hitPoints = hitPoints;       // 错误：形参赋给自己
        this->m_hitPoints = hitPoints;  // 正确：左边是成员
    }

    // 方法链：必须返回引用，否则改的是副本
    Monster& takeDamage(int damage)
    {
        m_hitPoints -= damage;
        if (m_hitPoints < 0) m_hitPoints = 0;
        return *this;
    }

    Monster& heal(int amount)
    {
        m_hitPoints += amount;
        return *this;
    }

    constexpr std::string_view getTypeString() const
    {
        switch (m_type)
        {
            case dragon:   return "dragon";
            case goblin:   return "goblin";
            case ogre:     return "ogre";
            case orc:      return "orc";
            case skeleton: return "skeleton";
            case troll:    return "troll";
            case vampire:  return "vampire";
            case zombie:   return "zombie";
            default:       return "???";
        }
    }

    void print() const
    {
        std::cout << m_name << " the " << getTypeString();
        if (m_hitPoints <= 0)
            std::cout << " is dead.\n";
        else
            std::cout << " has " << m_hitPoints
                      << " hit points and says " << m_roar << ".\n";
    }

    // 静态成员函数：没有 this，只能访问静态成员
    static int getAliveCount()
    {
        return s_alive;
    }

    // 友元：精确开后门，让外部函数能直接访问私有成员
    friend void printMonster(const Monster& m);
};

// 友元函数定义（普通全局函数，不是成员）
void printMonster(const Monster& m)
{
    std::cout << "[友元] " << m.m_name << " the " << m.getTypeString()
              << " has " << m.m_hitPoints << " hp\n";
}

// ------------------------------------------------------------
// 3. 无状态服务用命名空间
// ------------------------------------------------------------
namespace MonsterGenerator
{
    std::string_view getName(int n)
    {
        switch (n)
        {
            case 0: return "Blarg";
            case 1: return "Moog";
            case 2: return "PkShy";
            case 3: return "Gnash";
            case 4: return "Gruu";
            case 5: return "Bones";
            default: return "???";
        }
    }

    std::string_view getRoar(int n)
    {
        switch (n)
        {
            case 0: return "*ROAR*";
            case 1: return "*rawr*";
            case 2: return "*screech*";
            case 3: return "*growl*";
            case 4: return "*hiss*";
            case 5: return "*clatter*";
            default: return "???";
        }
    }

    Monster generate()
    {
        static std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> typeDist(0, Monster::maxMonsterTypes - 1);
        std::uniform_int_distribution<int> nameDist(0, 5);
        std::uniform_int_distribution<int> hpDist(1, 100);

        return Monster{
            static_cast<Monster::Type>(typeDist(rng)),
            getName(nameDist(rng)),
            getRoar(nameDist(rng)),
            hpDist(rng)
        };
    }
}

// ------------------------------------------------------------
// 4. 主函数
// ------------------------------------------------------------
int main()
{
    std::cout << "===== 1. 静态成员：程序刚启动 =====\n";
    std::cout << "当前活着的怪物数量: " << Monster::getAliveCount() << '\n';

    std::cout << "\n===== 2. 创建怪物 + 嵌套类型 =====\n";
    Monster skeleton{Monster::skeleton, "Bones", "*clatter*", 4};
    skeleton.print();

    std::cout << "\n===== 3. this 解决命名冲突 =====\n";
    skeleton.setHitPoints(10);
    skeleton.print();

    std::cout << "\n===== 4. 方法链 =====\n";
    skeleton.takeDamage(3).heal(5).takeDamage(2);
    skeleton.print();

    std::cout << "\n===== 5. 友元函数 =====\n";
    printMonster(skeleton);

    std::cout << "\n===== 6. 静态成员函数 =====\n";
    {
        Monster m2{Monster::dragon, "Smaug", "*ROAR*", 100};
        Monster m3{Monster::zombie, "Bob", "*groan*", 20};
        std::cout << "现在活着的怪物数量: " << Monster::getAliveCount() << '\n';
    }
    std::cout << "离开作用域后: " << Monster::getAliveCount() << '\n';

    std::cout << "\n===== 7. 命名空间生成随机怪物 =====\n";
    Monster randomMonster{MonsterGenerator::generate()};
    randomMonster.print();

    std::cout << "\n===== 8. 类模板 Array =====\n";
    Array<int> intArr;
    intArr.set(0, 42);
    intArr.set(1, 100);
    std::cout << "Array<int>[0] = " << intArr.get(0) << '\n';
    std::cout << "Array<int>[1] = " << intArr.get(1) << '\n';

    Array<std::string> strArr;
    strArr.set(0, "hello");
    std::cout << "Array<string>[0] = " << strArr.get(0) << '\n';

    std::cout << "\n===== 全部演示结束 =====\n";
    return 0;
}