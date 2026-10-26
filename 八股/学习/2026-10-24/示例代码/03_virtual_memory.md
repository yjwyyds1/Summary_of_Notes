# 03_virtual_memory — 完整注释源码

← [[学习/2026-10-24/示例代码/README|代码索引]] · [[学习/2026-10-24/03-虚拟内存|主题详解]]

以下代码与同名 .cpp 逐字一致；完整编译与输出见 [[学习/2026-10-24/示例验证]]。

```cpp
// C++17 算术模型：把虚拟地址拆成“虚拟页号 + 页内偏移”，再查示意页表。
// 这不是操作系统页表，也不读取真实物理地址；页大小 4096 只是本题给定值。
#include <cassert>       // assert：核对算式。
#include <cstdint>       // uint64_t：明确使用非负整数表示示意地址。
#include <iostream>      // cout：显示翻译结果。
#include <unordered_map> // unordered_map：保存“虚拟页号 -> 物理页框号”的示意映射。

int main() {
    constexpr std::uint64_t page_size = 4096; // 本例约定 4 KiB 页。
    constexpr std::uint64_t virtual_address = 0x1234;
    const std::uint64_t virtual_page = virtual_address / page_size; // 高位选页。
    const std::uint64_t offset = virtual_address % page_size;       // 低位页内偏移。
    // 虚拟第 1 页映到物理第 7 页；真实操作系统有权限位和多级页表等更多信息。
    const std::unordered_map<std::uint64_t, std::uint64_t> page_table{{1, 7}};
    const auto found = page_table.find(virtual_page); // 不存在时不能解引用 end()。
    assert(found != page_table.end());
    const std::uint64_t physical_address = found->second * page_size + offset;
    assert(virtual_page == 1 && offset == 0x234 && physical_address == 0x7234);
    std::cout << "virtual page=" << virtual_page << " offset=" << offset
              << " -> model physical address=0x" << std::hex << physical_address << '\n';
}
```
