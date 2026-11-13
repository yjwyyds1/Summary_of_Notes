// 目的：演示同一个业务请求 ID 的重复提交只生成一次订单。
// 前置：这是单进程内存模型；真实系统需持久存储、原子提交与结果查询。
// 步骤：首次处理保存 ID 到订单号映射；同 ID 再来时返回原订单号。
#include <cassert>      // assert：验证重复请求只生成一单。
#include <map>         // map：保存请求 ID 到处理结果的映射。
#include <string>      // string：请求 ID 的简单表示。
#include <stdexcept>   // invalid_argument：拒绝空 ID。

class OrderService {
    std::map<std::string, int> completed_; // 已完成请求的结果索引。
    int next_order_ = 1;                    // 教学用自增订单号。
public:
    int submit(const std::string& request_id) {
        if (request_id.empty()) throw std::invalid_argument("request id required");
        const auto found = completed_.find(request_id); // 先查是否已经执行过。
        if (found != completed_.end()) return found->second; // 重试返回同一结果。
        const int id = next_order_++;
        completed_.emplace(request_id, id); // 真实服务应与创建订单原子且持久提交。
        return id;
    }
    std::size_t count() const { return completed_.size(); }
};

int main() {
    OrderService service;
    const int first = service.submit("req-7"); // 第一次提交。
    const int retry = service.submit("req-7"); // 模拟超时后的同 ID 重试。
    assert(first == retry && service.count() == 1);
    assert(service.submit("req-8") != first); // 不同 ID 才是新请求。
}
