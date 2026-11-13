// 目的：在键互异的二叉搜索树中找中序后继。
// 前置：key 确实在树中；每个结点左子树的键小、右子树的键大。
// 步骤：搜索时记较大的祖先；目标若有右子树，改取右子树最左结点。
#include <cassert> // assert：验证后继及最大键无后继的边界。

struct Node { int key; Node* left; Node* right; }; // 左右指针指向孩子或 nullptr。

const Node* successor(const Node* root, int key) { // const Node*：只读结点，不修改树。
    const Node* candidate = nullptr; // 搜索路径上比 key 大的最近祖先。
    const Node* cur = root;
    while (cur && cur->key != key) {
        if (key < cur->key) { candidate = cur; cur = cur->left; }
        else cur = cur->right;
    }
    if (!cur) return nullptr; // 本例顺带为不存在的 key 返回空，但不作通用约定。
    if (cur->right) {          // 右子树所有键都大于目标。
        cur = cur->right;
        while (cur->left) cur = cur->left; // 取其中最小的键。
        return cur;
    }
    return candidate; // 没有右子树，取最近的较大祖先。
}

int main() {
    Node n1{1, nullptr, nullptr}, n3{3, nullptr, nullptr};
    Node n2{2, &n1, &n3}, n5{5, nullptr, nullptr};
    Node n4{4, &n2, &n5}; // 中序序列是 1,2,3,4,5。
    assert(successor(&n4, 2) == &n3);      // 有右子树。
    assert(successor(&n4, 3) == &n4);      // 没有右子树。
    assert(successor(&n4, 5) == nullptr);  // 最大键无后继。
}
