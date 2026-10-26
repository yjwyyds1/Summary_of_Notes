//
// Created by 20675 on 2026/10/23.
//
// ============================================================
// 第 23 章 完整示范：对象之间的关系（Composition / Aggregation /
// Association / Dependency / Container / initializer_list）
// 目标：让你以后独自看代码时，能立刻分辨出“这是什么关系、为什么这样写”
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <memory>          // 现代 C++ 推荐用智能指针
#include <initializer_list>
#include <fstream>

// ------------------------------------------------------------
// 1. 组合（Composition）—— 最强的整体-部分关系（同生共死）
// ------------------------------------------------------------
// 设计目的：
//   把“部分”完全交给“整体”管理。整体活着部分就活着，整体死了部分也死。
// 好处：
//   1. 生命周期完全自动，几乎不可能内存泄漏或悬空指针。
//   2. 封装性最好，外部无法直接碰部件。
//   3. 代码最简单，不需要手写 delete。
// 适用场景：
//   部件是整体的“内在组成部分”，离开整体就没有独立意义（心脏、位置、名字…）。

// 部件类：Point2D —— 它完全不知道自己会被谁使用（单向）
class Point2D {
private:
    int m_x{};
    int m_y{};
public:
    Point2D(int x = 0, int y = 0) : m_x{x}, m_y{y} {}
    void print() const {
        std::cout << "(" << m_x << ", " << m_y << ")";
    }
    // 注意：Point2D 里没有任何 Creature 的痕迹 → 满足“部分不知道整体”
};

// 整体类：Creature —— 用值成员实现组合（最推荐的写法）
class Creature {
private:
    std::string m_name;     // 组合：string 是 Creature 的一部分
    int         m_health;   // 基本类型也算“部分”
    Point2D     m_position; // 组合：位置是 Creature 的一部分（值成员）

public:
    // 构造函数：部件随整体一起构造（初始化列表）
    Creature(const std::string& name, int health, int x, int y)
        : m_name{name}, m_health{health}, m_position{x, y}
    {
        // 为什么用初始化列表？
        // 1. 效率更高（直接构造，而不是先默认再赋值）
        // 2. 对于 const / 引用成员必须用初始化列表
        // 3. 保证“部件和整体同时出生”
    }

    void moveTo(int x, int y) {
        // 局部临时对象也是组合思想：用完即弃
        m_position = Point2D{x, y};
    }

    void print() const {
        std::cout << m_name << " has " << m_health << " hp at ";
        m_position.print();
        std::cout << '\n';
    }

    // 析构函数可以不写：编译器会自动调用所有成员的析构
    // 这就是组合最大的好处 —— 零手动内存管理
};

// ------------------------------------------------------------
// 2. 组合的另一种实现：指针 + 整体负责 new/delete（不得已才用）
// ------------------------------------------------------------
// 什么时候用？
//   1. 需要多态（基类指针指向不同派生类）
//   2. 需要打破头文件循环依赖
// 设计目的：仍然保持“整体完全掌管生命周期”，所以还是组合。
// 好处：能做多态；坏处：要自己处理三法则（Rule of Three/Five）。

class Injury {
public:
    std::string where;
    Injury(const std::string& w) : where{w} {}
};

class Patient {
private:
    // 用 unique_ptr 代替裸指针，既保持组合语义，又自动 delete
    std::vector<std::unique_ptr<Injury>> m_injuries;

public:
    void addInjury(const std::string& location) {
        // 整体负责创建 → 组合的标志
        m_injuries.push_back(std::make_unique<Injury>(location));
    }

    // 析构函数可以不写：unique_ptr 会自动释放
    // 如果用裸指针，就必须自己写 ~Patient() { for(...) delete p; }
    // 并且要处理拷贝构造/赋值（三法则），非常麻烦。
    // 这就是为什么现代 C++ 强烈推荐 unique_ptr 做组合。
};

// ------------------------------------------------------------
// 3. 聚合（Aggregation）—— 松散的整体-部分（借用，不负责生死）
// ------------------------------------------------------------
// 设计目的：
//   整体“拥有”部件的使用权，但不拥有其生命周期。
//   部件可以同时属于多个整体，整体销毁后部件还活着。
// 好处：
//   1. 支持共享（一个学生可以同时在多个班级）。
//   2. 部件可以独立存在、独立测试。
// 代价：
//   必须保证“部件比整体活得久”，否则产生悬空指针。

class Student {
private:
    std::string m_name;
public:
    Student(const std::string& name) : m_name{name} {}
    const std::string& getName() const { return m_name; }
    // Student 完全不知道 Course 的存在 → 单向
};

class Course {
private:
    std::string m_name;
    // 只存指针，不 new、不 delete → 聚合的标志
    std::vector<Student*> m_students;

public:
    Course(const std::string& name) : m_name{name} {}

    void addStudent(Student* s) {
        // 只是登记，不拥有所有权
        m_students.push_back(s);
    }

    // 析构函数故意不 delete 学生！
    // 因为学生不是 Course 创建的，也不能由 Course 销毁。
    ~Course() = default;

    void printRoster() const {
        std::cout << m_name << " roster:\n";
        for (Student* s : m_students)
            std::cout << "  " << s->getName() << '\n';
    }
};

// ------------------------------------------------------------
// 4. 关联（Association）—— “使用”关系（既非整体也非部分）
// ------------------------------------------------------------
// 设计目的：
//   两个对象长期“认识”对方，但谁也不属于谁。
//   可以动态建立/解除，可以是单向或双向。
// 好处：
//   灵活，支持可选关系（司机可以暂时没有车）。
// 注意：
//   关联和聚合的最大区别是：关联不是“整体-部分”关系。

class Car {
private:
    std::string m_model;
public:
    Car(const std::string& model) : m_model{model} {}
    void drive() const {
        std::cout << m_model << " is driving\n";
    }
};

class Driver {
private:
    std::string m_name;
    Car* m_car{};   // 可以是 nullptr（还没分到车）

public:
    Driver(const std::string& name) : m_name{name} {}

    // 关联可以动态建立、动态解除
    void assignCar(Car* car) { m_car = car; }

    void commute() const {
        if (m_car)
            m_car->drive();
        else
            std::cout << m_name << " takes the bus\n";
    }
};

// ------------------------------------------------------------
// 5. 双向关联（关联独有的特点）
// ------------------------------------------------------------
// 设计目的：老师知道学生，学生也知道老师。
// 代价：必须两边同时维护一致性，否则数据会乱。
// 实践建议：能单向就别双向，降低复杂度。

class Student2; // 前置声明，打破循环依赖

class Teacher {
private:
    std::string m_name;
    std::vector<Student2*> m_students;
public:
    Teacher(const std::string& n) : m_name{n} {}
    void addStudent(Student2* s); // 定义在后面
};

class Student2 {
private:
    std::string m_name;
    std::vector<Teacher*> m_teachers;
public:
    Student2(const std::string& n) : m_name{n} {}
    void addTeacher(Teacher* t) {
        m_teachers.push_back(t);
    }
};

// 实现双向维护
void Teacher::addStudent(Student2* s) {
    m_students.push_back(s);
    s->addTeacher(this); // 同时告诉学生“你有我这个老师”
}

// ------------------------------------------------------------
// 6. 依赖（Dependency）—— 用完即走的临时关系
// ------------------------------------------------------------
// 设计目的：
//   只是临时借用另一个对象完成一件事，用完就没关系了。
// 与关联的区别：
//   关联 = 长期持有（成员变量）
//   依赖 = 临时使用（参数或局部对象）

class File {
public:
    void write(const std::string& content) {
        std::ofstream fout{"save.txt"};
        fout << content;
        std::cout << "Saved to disk.\n";
    }
};

class TextEditor {
private:
    std::string m_content{"Hello, world!"};
public:
    // 形式 1：作为参数传入 → 依赖
    void save(File& f) {
        f.write(m_content); // 借用一下就完事
    }

    // 形式 2：局部创建、用完即销毁 → 也是依赖
    void saveToTemp() {
        File temp;          // 临时对象
        temp.write(m_content);
    } // temp 在此析构，与 TextEditor 完全无关
};

// ------------------------------------------------------------
// 7. 容器类（Container Classes）
// ------------------------------------------------------------
// 值容器 = 组合（存副本，容器拥有生命周期）
// 引用容器 = 聚合（存指针，外部拥有生命周期）

class NameList { // 值容器 → 组合
private:
    std::vector<std::string> m_names; // 存副本
public:
    void add(const std::string& n) {
        m_names.push_back(n); // 拷贝一份进来
    }
    void print() const {
        for (const auto& n : m_names)
            std::cout << n << '\n';
    }
    // 容器销毁时，所有副本自动销毁 → 组合语义
};

class Registry { // 引用容器 → 聚合
private:
    std::vector<Student*> m_students; // 只存指针
public:
    void add(Student* s) {
        m_students.push_back(s); // 不拥有所有权
    }
    // 析构时不 delete → 聚合语义
};

// ------------------------------------------------------------
// 8. std::initializer_list —— 让类支持花括号初始化
// ------------------------------------------------------------
// 设计目的：让用户可以写出 IntArray a{1, 2, 3}; 这种自然语法。
// 好处：接口更友好，和标准库容器风格一致。

class IntArray {
private:
    int m_length{};
    int* m_data{};
public:
    IntArray() = default;

    // 接受 {1, 2, 3} 这样的列表
    IntArray(std::initializer_list<int> list)
        : IntArray{} // 委托默认构造
    {
        m_length = static_cast<int>(list.size());
        if (m_length > 0) {
            m_data = new int[static_cast<std::size_t>(m_length)];
            int i = 0;
            for (int e : list)   // initializer_list 可以像容器一样遍历
                m_data[i++] = e;
        }
    }

    ~IntArray() { delete[] m_data; }

    // 也支持赋值
    IntArray& operator=(std::initializer_list<int> list) {
        delete[] m_data;
        m_length = static_cast<int>(list.size());
        m_data = new int[static_cast<std::size_t>(m_length)];
        int i = 0;
        for (int e : list)
            m_data[i++] = e;
        return *this;
    }

    void print() const {
        for (int i = 0; i < m_length; ++i)
            std::cout << m_data[i] << ' ';
        std::cout << '\n';
    }
};

// ------------------------------------------------------------
// 主函数：把所有关系演示一遍
// ------------------------------------------------------------
int main() {
    std::cout << "===== 1. 组合（Composition）演示 =====\n";
    {
        Creature orc{"Orc", 40, 3, 5};
        orc.print();          // Orc has 40 hp at (3, 5)
        orc.moveTo(10, 20);
        orc.print();          // Orc has 40 hp at (10, 20)
    } // orc 销毁时，m_name、m_position 自动析构 → 组合的威力

    std::cout << "\n===== 2. 聚合（Aggregation）演示 =====\n";
    {
        Student alice{"Alice"}, bob{"Bob"}; // 学生独立存在

        Course math{"Math 101"};
        Course physics{"Physics 201"};

        math.addStudent(&alice);
        math.addStudent(&bob);
        physics.addStudent(&alice); // 同一个学生可以属于多门课

        math.printRoster();
        physics.printRoster();
    } // 课程先死，学生还活着 → 典型聚合

    std::cout << "\n===== 3. 关联（Association）演示 =====\n";
    {
        Car car{"Tesla Model 3"};
        Driver driver{"Tom"};

        driver.commute();     // 还没车，坐公交
        driver.assignCar(&car);
        driver.commute();     // 有车了，开车
    }

    std::cout << "\n===== 4. 依赖（Dependency）演示 =====\n";
    {
        TextEditor editor;
        File file;
        editor.save(file);        // 参数依赖
        editor.saveToTemp();      // 局部对象依赖
    }

    std::cout << "\n===== 5. 值容器（组合）与引用容器（聚合） =====\n";
    {
        std::string s{"Alice"};
        NameList list;
        list.add(s);
        s = "changed";
        list.print();             // 仍是 Alice（值容器存的是副本）
    }

    std::cout << "\n===== 6. initializer_list 演示 =====\n";
    {
        IntArray a{1, 2, 3};      // 花括号初始化
        a.print();                // 1 2 3
        a = {4, 5, 6, 7};         // 花括号赋值
        a.print();                // 4 5 6 7
    }

    std::cout << "\n===== 全部演示结束 =====\n";
    return 0;
}