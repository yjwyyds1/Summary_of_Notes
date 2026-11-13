# 2026-11-02 · GET、POST、PUT 的语义和幂等性：注释源码

先修：[[学习/2026-11-02/04-HTTP方法与幂等|对应专题]]。此程序为独立 C++17 教学示例，成功时 `assert` 无输出。

```cpp
// 目的：用状态变化说明“重复请求的预期效果”与“每次响应相同”是两回事。
// 前置：这里只模拟 HTTP 方法的一个业务例子，不解析或发送任何 HTTP 报文。
// 步骤：重复 PUT 替换同一资源；重复 POST 新建两条资源。
#include <cassert> // assert：核对重复执行后的状态。
#include <map>     // map：保存资源 ID 与数值。

struct Store {
    std::map<int, int> values; // key 是资源 ID，value 是资源状态。
    int next_id = 1;           // POST 示例由服务端分配新 ID。

    void put(int id, int value) { values[id] = value; } // 目标 ID 相同就替换。
    int post(int value) {            // 此业务定义为“每次创建新资源”。
        const int id = next_id++;
        values[id] = value;
        return id;
    }
};

int main() {
    Store s;
    s.put(7, 9);               // 第一次把资源 7 写成 9。
    s.put(7, 9);               // 同一请求重复，最终仍只有资源 7。
    assert(s.values.size() == 1 && s.values.at(7) == 9);
    const int a = s.post(9);   // 第一次创建。
    const int b = s.post(9);   // 超时后盲目重试，会再创建一条。
    assert(a != b && s.values.size() == 3);
}
```

观察：先自己预测每个断言检查什么，再按 [[学习/2026-11-02/示例验证|验证记录]] 编译运行。
