#include <cassert>
#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>

std::optional<int> parseVolume(std::string_view text) {
    if (text.empty()) return std::nullopt;
    int value = 0;
    const auto end = text.data() + text.size();
    const auto [ptr, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc{} || ptr != end || value < 0 || value > 100)
        return std::nullopt;
    return value;
}

int main() {
    assert(parseVolume("0") == 0);
    assert(parseVolume("100") == 100);
    for (const auto input : {"", "-1", "101", "12x", " 1", "999999999999999999"})
        assert(!parseVolume(input));
    std::cout << "contract OK\n";
}
