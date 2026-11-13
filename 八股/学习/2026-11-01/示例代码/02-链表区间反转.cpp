// 目的：原地反转单链表中从 left 到 right 的结点（位置从 1 开始）。
// 前置：链表无环，且 1 <= left <= right <= 链表长度。
// 步骤：定位区间前驱，每次把当前结点后面的一个结点搬到区间前部。
#include <cassert> // assert：验证结点顺序和新链表头。

struct Node { int value; Node* next; }; // next 保存下一结点地址；nullptr 表示结尾。

Node* reverse_between(Node* head, int left, int right) {
    Node dummy{0, head}; // 虚拟头只在函数调用期间存在，不属于返回链表。
    Node* pre = &dummy;  // * 表示指针变量；&dummy 取得局部对象的地址。
    for (int pos = 1; pos < left; ++pos) pre = pre->next; // -> 访问指针所指结点。
    Node* cur = pre->next; // cur 始终是已经反转部分的尾结点。
    for (int i = 0; i < right - left; ++i) {
        Node* move = cur->next;   // 先保存要搬的结点，否则改指针后会丢链。
        cur->next = move->next;   // 从原位置摘掉 move。
        move->next = pre->next;   // move 指向当前区间首结点。
        pre->next = move;         // 区间首结点改为 move。
    }
    return dummy.next; // 返回的是实际结点地址，不是 dummy 地址。
}

int main() {
    Node e{5, nullptr}, d{4, &e}, c{3, &d}, b{2, &c}, a{1, &b};
    Node* head = reverse_between(&a, 2, 4);
    assert(head == &a && a.next == &d && d.next == &c);
    assert(c.next == &b && b.next == &e && e.next == nullptr);
    head = reverse_between(head, 1, 2); // 覆盖“头结点改变”的边界。
    assert(head == &d && d.next == &a);
}
