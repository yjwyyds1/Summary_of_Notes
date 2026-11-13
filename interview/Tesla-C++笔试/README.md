# Tesla 信息娱乐 C++ 笔试实训

**第一步，学第 02 和 03 章，跑例程，然后关掉例程从空文件重写阻塞队列。** 能说出关闭时为什么唤醒全部线程、关闭后已有数据怎么处理，就进入第一套模拟。每章都有完整源文件，建议先预测行为，再运行，再动手改。

- [知识讲义：从接口到并发关闭](01-知识讲义.md)
- [三套原创模拟：规格、骨架、测试和计分](02-综合模拟题.md)
- [做完再看：提示和复盘](03-参考答案与复盘.md)
- [原来的备考速览和回信模板](Tesla-C++-Codility笔试准备.md)

## 准备权重

招聘邮件明确提到 80 分钟、任务拆解、设计、实现、编译和测试，并列出 C++17 线程、同步、异常、智能指针及边界。优先练“把给定接口做对”和“修已有实现”。Codility 公开支持单文件编程、算法及带既有代码的多文件项目；从邮件无法判断你这一场的题数、编辑器或是否另有算法题。这三套题是根据技能范围写的原创练习。 [Codility 题型与环境说明](https://support.codility.com/hc/en-us/articles/45339349251345-Creating-Your-Own-Content-Build-with-MCP)

你已有竞赛基础。算法保底留 20–30 分钟，检查哈希去重、排序后二分、前缀和、复杂度与溢出。工程部分的额外投入优先放在条件变量、对象生命周期、超时、锁外执行与测试上。 [Codility 公开算法课程](https://app.codility.com/programmers/lessons/)

## 运行

在 PowerShell 进入本目录。本机已用 GCC 13.1 的 C++17 模式验证。每个例程都可以单独在 IDE 打开，或者用下面的命令跑。脚本临时编译和执行，8 秒未退出会报 TIMEOUT。

```powershell
Set-Location -LiteralPath 'D:/Summary_of_Notes/interview/Tesla-C++笔试'
.\run.ps1
.\run.ps1 -Task 1 -Case fifo
.\run.ps1 -Task 2 -Case reentry
.\run.ps1 -Task 3
# 做完后可用相同用例验证参考实现
.\run.ps1 -Task 3 -Reference
```

手工编译单个例程的命令：

```powershell
g++ -std=c++17 -g -Wall -Wextra -Wpedantic -pthread '.\代码\03_blocking_queue.cpp' -o "$env:TEMP\tesla-queue.exe"
& "$env:TEMP\tesla-queue.exe"
```

如果 PowerShell 的执行策略拦截脚本，可直接使用上面的 g++ 命令；模拟题则进入对应目录，用 test.cpp 替换源文件。手工运行卡住时按 Ctrl+C。

## 学习安排

| 剩余时间 | 动手安排 |
|---|---|
| 约 3 小时 | 第 02、03、05 章和重写队列 45 分钟，模拟 1 做 50 分钟，模拟 3 做 80 分钟，剩下时间记录卡点 |
| 约 2 天 | 第一天跑 01–05 并做模拟 1、2。第二天练 06–08，做 80 分钟模拟 3，最后重做一个失败用例 |
| 约 3 天以上 | 每天一套，第二天重写前一天最不熟的函数。考前只看自己写错的地方和闭卷检查表 |

进场前用 [Codility 公开练习](https://app.codility.com/demo/take-sample-test/) 检查编辑器和 Run Code。正式题按邀请邮件独立完成。练习自评见模拟题，分数用于发现薄弱环节。

