#include <iostream>
#include <queue>

int main() {
    std::queue<int> numbers;
    numbers.push(0);
    numbers.push(42);
    while (!numbers.empty()) {
        int value = numbers.front();
        numbers.pop();
        std::cout << value << '\n';
    }
}
