#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <vector>

std::vector<int> firstOccurrences(const std::vector<int>& ids) {
    std::unordered_set<int> seen;
    std::vector<int> result;
    for (int id : ids)
        if (seen.insert(id).second) result.push_back(id);
    return result;
}

std::vector<long long> prefixSums(const std::vector<int>& values) {
    std::vector<long long> prefix(values.size() + 1);
    for (std::size_t i = 0; i < values.size(); ++i)
        prefix[i + 1] = prefix[i] + values[i];
    return prefix;
}

int main() {
    assert((firstOccurrences({7, -1, 7, 0, -1}) == std::vector<int>{7, -1, 0}));
    const auto prefix = prefixSums({1'500'000'000, 1'500'000'000, -2});
    assert(prefix[2] - prefix[0] == 3'000'000'000LL);
    assert(prefix[3] - prefix[1] == 1'499'999'998LL);
    const std::vector<int> sorted{-1, 3, 3, 8};
    const auto position = std::lower_bound(sorted.begin(), sorted.end(), 3);
    assert(position == sorted.begin() + 1);
    std::cout << "algorithm OK\n";
}
