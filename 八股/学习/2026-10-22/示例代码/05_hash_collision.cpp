// C++17 学习示例：让不同键故意得到同一个哈希值，观察桶与 rehash。
// 这只是冲突演示；生产代码不应故意把全部键散列到同一值。

#include <cassert>       // assert：验证两个键共桶、值仍可找到。
#include <cstddef>       // std::size_t：桶数量使用的无符号计数类型。
#include <iostream>      // std::cout：显示冲突与负载因子。
#include <string>        // std::string：映射值的类型。
#include <unordered_map> // std::unordered_map：提供 bucket、rehash、find。

struct ConstantHash {
    // 容器调用函数对象的 operator() 来取得键的哈希值。
    // const：不修改哈希器；noexcept：不从这里抛异常；返回类型 size_t。
    std::size_t operator()(int) const noexcept { return 1; }
};

int main() {
    std::unordered_map<int, std::string, ConstantHash> table;
    table.emplace(1, "one"); // 建立“键 1 -> one”。
    table.emplace(2, "two"); // 键不同，但哈希器仍返回相同的 1。

    const bool collide = table.bucket(1) == table.bucket(2);
    assert(collide); // 同哈希值的两个键会归入同一桶。
    assert(table.find(1)->second == "one");
    assert(table.find(2)->second == "two");

    const std::size_t before = table.bucket_count();
    auto before_rehash = table.find(1); // 这份迭代器在 rehash 后不可继续用。
    std::string& kept_value = before_rehash->second; // 元素引用在 rehash 后仍有效。
    table.rehash(before * 2 + 1); // 请求更多桶；具体桶数由实现选择。
    assert(table.bucket_count() > before);
    assert(kept_value == "one");
    // 不再使用 before_rehash；重新查找后取得新的有效迭代器。
    assert(table.find(1)->second == "one");

    std::cout << "same bucket: " << std::boolalpha << collide << '\n';
    std::cout << "load factor: " << table.load_factor() << '\n';
    // 负载因子约为 size / bucket_count；数值受库选择的桶数影响。
}
