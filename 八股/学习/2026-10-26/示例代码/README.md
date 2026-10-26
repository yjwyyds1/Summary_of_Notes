# 2026-10-26 示例代码

这七份是 Linux/POSIX C++17 教学示例。先读专题，再看同名 Markdown 阅读版；每个阅读版嵌入与 `.cpp` 完全一致的完整源码。

| 专题 | 阅读版 | 源码 |
| --- | --- | --- |
| 文件描述符、打开文件描述与共享偏移 | [[学习/2026-10-26/示例代码/01_fd_offset\|Markdown]] | [[学习/2026-10-26/示例代码/01_fd_offset.cpp\|cpp]] |
| read/write 为什么要处理短读写 | [[学习/2026-10-26/示例代码/02_exact_io\|Markdown]] | [[学习/2026-10-26/示例代码/02_exact_io.cpp\|cpp]] |
| write 返回与 fsync：数据何时算持久 | [[学习/2026-10-26/示例代码/03_fsync\|Markdown]] | [[学习/2026-10-26/示例代码/03_fsync.cpp\|cpp]] |
| mmap 与 read：按访问方式选接口 | [[学习/2026-10-26/示例代码/04_mmap_read\|Markdown]] | [[学习/2026-10-26/示例代码/04_mmap_read.cpp\|cpp]] |
| 临时文件、rename 与目录 fsync | [[学习/2026-10-26/示例代码/05_atomic_replace\|Markdown]] | [[学习/2026-10-26/示例代码/05_atomic_replace.cpp\|cpp]] |
| 预写日志 WAL、提交点与恢复 | [[学习/2026-10-26/示例代码/06_wal_order\|Markdown]] | [[学习/2026-10-26/示例代码/06_wal_order.cpp\|cpp]] |
| 练习：遇到不完整日志尾部怎样恢复 | [[学习/2026-10-26/示例代码/07_recover_tail\|Markdown]] | [[学习/2026-10-26/示例代码/07_recover_tail.cpp\|cpp]] |

编译命令、实际输出和验证边界见 [[学习/2026-10-26/示例验证]]。
