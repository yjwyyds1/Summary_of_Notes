---
date: 2026-10-20
updated: 2026-10-21
cpp_standard: C++17
type: 示例代码导航
tags:
  - 学习/C++/代码阅读
---

# 2026-10-20 · 示例代码阅读目录

← [[学习/学习导航/2026-10-20-学习导航|当天学习入口]]

优先点下面的 `.md` 阅读笔记：每篇都直接展示与同名 `.cpp` 逐字一致的完整中文注释代码，并附输出观察和折叠自测答案。`.cpp` 保留给编辑器、编译器使用；不需要安装 Obsidian 插件来读这些笔记。

## 建议顺序

| 阅读笔记 | 这份代码主要学什么 | 原始源文件 |
| --- | --- | --- |
| [[学习/2026-10-20/示例代码/00_objects\|00_objects.md]] | 对象、指针与引用；const 借用；构造/析构与作用域 | [[学习/2026-10-20/示例代码/00_objects.cpp\|00_objects.cpp]] |
| [[学习/2026-10-20/示例代码/01_lifetime\|01_lifetime.md]] | 局部/static/动态对象；reset；原始存储与 placement new | [[学习/2026-10-20/示例代码/01_lifetime.cpp\|01_lifetime.cpp]] |
| [[学习/2026-10-20/示例代码/02_raii\|02_raii.md]] | RAII；异常栈展开；成员构造失败；清理与回滚 | [[学习/2026-10-20/示例代码/02_raii.cpp\|02_raii.cpp]] |
| [[学习/2026-10-20/示例代码/03_ownership\|03_ownership.md]] | 借用；unique_ptr 移动/release/reset；shared_ptr 与 weak_ptr | [[学习/2026-10-20/示例代码/03_ownership.cpp\|03_ownership.cpp]] |
| [[学习/2026-10-20/示例代码/04_threads\|04_threads.md]] | lambda 按值捕获；shared_ptr 副本；mutex；join | [[学习/2026-10-20/示例代码/04_threads.cpp\|04_threads.cpp]] |
| [[学习/2026-10-20/示例代码/04_publish\|04_publish.md]] | lambda 引用捕获；shared_ptr 原子访问；const 快照；join | [[学习/2026-10-20/示例代码/04_publish.cpp\|04_publish.cpp]] |
| [[学习/2026-10-20/示例代码/05_move\|05_move.md]] | 复制/移动构造与赋值；&&；const；std::move；返回值 | [[学习/2026-10-20/示例代码/05_move.cpp\|05_move.cpp]] |
| [[学习/2026-10-20/示例代码/06_vector\|06_vector.md]] | reserve/resize；失效；索引与身份；unique_ptr 槽位；erase | [[学习/2026-10-20/示例代码/06_vector.cpp\|06_vector.cpp]] |

04 的两份分开看：先用 mutex 保护同一个 Counter 的可变数据，再看原子替换 shared_ptr 槽位以发布只读快照。两者解决的问题不同。

## 如何运行

每个 `.cpp` 都有自己的 main，应分别编译成独立程序，不要把 8 份文件一次链接到同一个可执行文件。基线是 C++17；保留断言时不要额外定义 `NDEBUG`。

先在终端进入本目录，例如 Windows PowerShell：

```powershell
Set-Location -LiteralPath 'D:\Summary_of_Notes\八股\学习\2026-10-20\示例代码'
```

如果使用 Visual Studio 的 Developer PowerShell，已能找到 `cl`，以第一份为例：

```powershell
cl /nologo /EHsc /std:c++17 /utf-8 00_objects.cpp /Fe:00_objects.exe
.\00_objects.exe
```

把 `00_objects` 换成想观察的文件名，即可分别构建其他例子。`/utf-8` 让编译器按 UTF-8 读取含中文注释的源文件。

使用已配置好的 GCC/Clang 时，可采用对应的 C++17 命令；例如在支持 `-pthread` 的环境编译线程示例：

```sh
g++ -std=c++17 -Wall -Wextra -pedantic -pthread 04_threads.cpp -o 04_threads
./04_threads
```

编译器必须已经安装且可由终端找到。这些是操作示例，实际编译器、检查范围与结果见 [[学习/2026-10-20/示例验证|示例验证]]；这里没有把预期输出写成已执行的测试结论。

## 阅读时先问的四件事

1. 这一行是创建对象、给已有对象赋值，还是只绑定一个引用？
2. 指针/引用观察哪个对象，谁负责销毁它，它在这里还活着吗？
3. 有没有共享数据被并发访问，哪把锁、哪个原子接口或哪个 join 建立了所需关系？
4. 输出或断言检验的具体性质是什么，是否误把一次结果推成了对所有程序的结论？

每篇最后有 5 个自测问题，关键答案默认折叠。先口述再展开；这些是代码阅读辅助，不替代 [[八股测试/2026-10-20|当天正式自测页]]，也不代表已经掌握。
