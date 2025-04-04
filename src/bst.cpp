#include "bst.hpp"

BSTNode::BSTNode(uint16_t idx, const std::array<uint8_t, L>& cont)
    : index(idx), content(cont), left(nullptr), right(nullptr) {
}

void BSTNode::print() {
  std::cout << "Node " << index << " content: ";
  for (auto& c : content) {
    std::cout << static_cast<int>(c) << " ";
  }
  std::cout << std::endl;
}

bool compare_content_less(const std::array<uint8_t, L>& content_one,
                          const std::array<uint8_t, L>& content_two) {
  for (int i = 0; i < L; ++i) {
    if (content_one[i] < content_two[i]) {
      return true;
    }
    if (content_one[i] > content_two[i]) {
      return false;
    }
  }
  return false;
}

BSTree::BSTree() : root(nullptr) {
}

BSTree::~BSTree() {
  destroy_recursive(root);
}

void BSTree::destroy_recursive(BSTNode* node) {
  if (node) {
    destroy_recursive(node->left);
    destroy_recursive(node->right);
    delete node;
  }
}

void BSTree::insert_data(uint16_t index,
                         const std::array<uint8_t, L>& content) {
  BSTNode* new_node = new BSTNode(index, content);

  if (root == nullptr) {
    root = new_node;
    return;
  }

  BSTNode* prev_node = nullptr;
  BSTNode* current_node = root;
  while (current_node != nullptr) {
    prev_node = current_node;
    if (compare_content_less(new_node->content, current_node->content)) {
      current_node = current_node->left;
    } else {
      current_node = current_node->right;
    }
  }

  if (compare_content_less(new_node->content, prev_node->content)) {
    prev_node->left = new_node;
  } else {
    prev_node->right = new_node;
  }
}

void BSTree::print_recursive_preorder(BSTNode* node) {
  if (node) {
    node->print();
    print_recursive_preorder(node->left);
    print_recursive_preorder(node->right);
  }
}

void remove_node(uint16_t index) {
  // Implement the remove node logic here
}

// void BSTree::find_longes_prefix() {
//   // Implement the find longest prefix logic here
// }