// ============================================================
//  第24章 继承（Inheritance）——完整教学代码（已修复构造函数）
//  编译：g++ -std=c++17 -Wall -o inheritance inheritance.cpp && ./inheritance
// ============================================================

#include <iostream>
#include <string>
#include <array>
#include <cstdlib>
#include <ctime>

// ============================================================
// 一、继承的本质：is-a
// ============================================================
class Fruit {
protected:
    std::string m_name;
    std::string m_color;

public:
    // 基类构造函数：必须接收名字和颜色
    Fruit(const std::string& name, const std::string& color)
        : m_name{ name }, m_color{ color }
    {}

    const std::string& getName() const { return m_name; }
    const std::string& getColor() const { return m_color; }

    void print() const {
        std::cout << "Fruit: " << m_name << " (" << m_color << ")\n";
    }
};

class Apple : public Fruit {
public:
    // 派生类构造：把固定名字 "apple" 和颜色转发给基类
    explicit Apple(const std::string& color = "red")
        : Fruit{ "apple", color }   // 关键：必须在初始化列表里调用基类构造
    {}

    void polish() {
        std::cout << "Polishing the " << m_name << "...\n";
    }
};

// 对比：组合（has-a）
class Engine {
public:
    void start() { std::cout << "Engine started\n"; }
};

class Car {
    Engine m_engine;   // 拥有一个引擎，不是“是一个”引擎
public:
    void start() { m_engine.start(); }
};

// ============================================================
// 二、多层继承（传递性）
// ============================================================
class GrannySmith : public Apple {
public:
    GrannySmith()
        : Apple{ "green" }   // 调用 Apple 的构造 → 最终调用 Fruit 的构造
    {}
};

// ============================================================
// 三、访问控制
// ============================================================
class BaseAccess {
private:
    int m_private{ 1 };

protected:
    int m_protected{ 2 };

public:
    int m_public{ 3 };

    void showPrivate() const {
        std::cout << "private = " << m_private << '\n';
    }
};

class DerivedAccess : public BaseAccess {
public:
    void test() {
        // m_private = 10;   // 编译错误：private 对派生类不可见
        m_protected = 20;    // OK
        m_public = 30;       // OK
        showPrivate();       // 通过公有接口间接访问
    }
};

// ============================================================
// 四、构造 / 析构顺序
// ============================================================
class BaseCtor {
public:
    BaseCtor() { std::cout << "  BaseCtor()\n"; }
    explicit BaseCtor(int x) { std::cout << "  BaseCtor(" << x << ")\n"; }
    ~BaseCtor() { std::cout << "  ~BaseCtor()\n"; }
};

class DerivedCtor : public BaseCtor {
public:
    explicit DerivedCtor(int y)
        : BaseCtor{ y }      // 显式指定基类构造，否则会找默认构造
    {
        std::cout << "  DerivedCtor(" << y << ")\n";
    }
    ~DerivedCtor() { std::cout << "  ~DerivedCtor()\n"; }
};

// ============================================================
// 五、名字遮蔽 + using
// ============================================================
class BaseHide {
public:
    void print() const { std::cout << "BaseHide::print()\n"; }
    void print(int x) const { std::cout << "BaseHide::print(" << x << ")\n"; }
protected:
    void secret() { std::cout << "secret\n"; }
};

class DerivedHide : public BaseHide {
public:
    void extra() { std::cout << "extra()\n"; }

    // 同名函数会遮蔽基类所有同名重载
    void print() const { std::cout << "DerivedHide::print()\n"; }

    // 把基类的重载集重新引入
    using BaseHide::print;

    // 提升访问级别
    using BaseHide::secret;
};

// ============================================================
// 六、多重继承
// ============================================================
class Flyer {
public:
    void fly() { std::cout << "flying\n"; }
};

class Swimmer {
public:
    void swim() { std::cout << "swimming\n"; }
};

class Duck : public Flyer, public Swimmer {};

// ============================================================
// 七、Fruit 家族（含 GrannySmith 的 protected 技巧）
// ============================================================
class Fruit2 {
protected:
    std::string m_name;
    std::string m_color;

public:
    Fruit2(const std::string& name, const std::string& color)
        : m_name{ name }, m_color{ color }
    {}

    const std::string& getName() const { return m_name; }
    const std::string& getColor() const { return m_color; }
};

class Apple2 : public Fruit2 {
protected:
    // 只有派生类能用的构造函数
    Apple2(const std::string& name, const std::string& color)
        : Fruit2{ name, color }
    {}

public:
    // 对外公开的构造：名字固定
    explicit Apple2(const std::string& color = "red")
        : Fruit2{ "apple", color }
    {}
};

class GrannySmith2 : public Apple2 {
public:
    GrannySmith2()
        : Apple2{ "granny smith apple", "green" }  // 走 protected 构造
    {}
};

// ============================================================
// 八、打怪游戏（整章综合）
// ============================================================
class Creature {
protected:
    std::string m_name;
    char m_symbol{};
    int m_health{};
    int m_damage{};
    int m_gold{};

public:
    // 主构造函数
    Creature(const std::string& name, char symbol, int health, int damage, int gold)
        : m_name{ name }, m_symbol{ symbol }, m_health{ health }
        , m_damage{ damage }, m_gold{ gold }
    {}

    // 拷贝构造（Monster 查表时需要）
    Creature(const Creature& other) = default;

    const std::string& getName() const { return m_name; }
    char getSymbol() const { return m_symbol; }
    int getHealth() const { return m_health; }
    int getDamage() const { return m_damage; }
    int getGold() const { return m_gold; }

    void reduceHealth(int amount) { m_health -= amount; }
    bool isDead() const { return m_health <= 0; }
    void addGold(int amount) { m_gold += amount; }
};

class Player : public Creature {
    int m_level{ 1 };

public:
    explicit Player(const std::string& name)
        : Creature{ name, '@', 10, 1, 0 }
    {}

    void levelUp() {
        ++m_level;
        ++m_damage;          // 直接改 protected 成员
    }
    int getLevel() const { return m_level; }
    bool hasWon() const { return m_level >= 20; }
};

class Monster : public Creature {
public:
    enum Type { dragon, orc, slime, max_types };

private:
    // 静态查表（C++17 inline static）
    inline static const Creature monsterData[max_types] {
        Creature{ "dragon", 'D', 20, 4, 100 },
        Creature{ "orc",    'o',  4, 2,  25 },
        Creature{ "slime",  's',  1, 1,  10 }
    };

public:
    explicit Monster(Type type)
        : Creature{ monsterData[type] }   // 调用拷贝构造
    {}

    static Monster getRandomMonster() {
        int num = std::rand() % max_types;
        return Monster{ static_cast<Type>(num) };
    }
};

// 战斗函数
void attackMonster(Player& player, Monster& monster) {
    if (player.isDead()) return;
    std::cout << "You hit the " << monster.getName()
              << " for " << player.getDamage() << " damage.\n";
    monster.reduceHealth(player.getDamage());

    if (monster.isDead()) {
        std::cout << "You killed the " << monster.getName() << ".\n";
        player.levelUp();
        std::cout << "You are now level " << player.getLevel() << ".\n";
        std::cout << "You found " << monster.getGold() << " gold.\n";
        player.addGold(monster.getGold());
    }
}

void attackPlayer(const Monster& monster, Player& player) {
    if (monster.isDead()) return;
    player.reduceHealth(monster.getDamage());
    std::cout << "The " << monster.getName()
              << " hit you for " << monster.getDamage() << " damage.\n";
}

void fightMonster(Player& player) {
    Monster monster{ Monster::getRandomMonster() };
    std::cout << "You have encountered a " << monster.getName()
              << " (" << monster.getSymbol() << ").\n";

    while (!monster.isDead() && !player.isDead()) {
        std::cout << "(R)un or (F)ight: ";
        char input{};
        std::cin >> input;

        if (input == 'R' || input == 'r') {
            if (std::rand() % 2 == 0) {
                std::cout << "You successfully fled.\n";
                return;
            } else {
                std::cout << "You failed to flee.\n";
                attackPlayer(monster, player);
                continue;
            }
        }

        if (input == 'F' || input == 'f') {
            attackMonster(player, monster);
            attackPlayer(monster, player);
        }
    }
}

// ============================================================
// 主函数
// ============================================================
int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "========== 1. is-a 关系 ==========\n";
    Apple a{ "red" };
    a.print();
    a.polish();

    std::cout << "\n========== 2. 构造/析构顺序 ==========\n";
    {
        DerivedCtor d{ 42 };
    }

    std::cout << "\n========== 3. 访问控制 ==========\n";
    DerivedAccess da;
    da.test();

    std::cout << "\n========== 4. 名字遮蔽 + using ==========\n";
    DerivedHide dh;
    dh.print();
    dh.print(99);
    dh.secret();

    std::cout << "\n========== 5. 多重继承 ==========\n";
    Duck duck;
    duck.fly();
    duck.swim();

    std::cout << "\n========== 6. Fruit 家族 ==========\n";
    Apple2 apple;
    GrannySmith2 gs;
    std::cout << apple.getName() << " is " << apple.getColor() << '\n';
    std::cout << gs.getName() << " is " << gs.getColor() << '\n';

    std::cout << "\n========== 7. 打怪游戏演示 ==========\n";
    Player player{ "Hero" };
    std::cout << "Welcome, " << player.getName() << "!\n";
    fightMonster(player);

    if (player.isDead()) {
        std::cout << "You died at level " << player.getLevel()
                  << " with " << player.getGold() << " gold.\n";
    } else {
        std::cout << "Survived! Level " << player.getLevel()
                  << ", gold " << player.getGold() << '\n';
    }

    std::cout << "\n========== 全部演示结束 ==========\n";
    return 0;
}