#include <stdint.h>

#include <array>
#include <iostream>
#include <stdexcept>  // For potential exceptions if needed

#define L 5

struct BSTNode {
  uint16_t index;
  std::array<uint8_t, L> content;
  BSTNode* left;
  BSTNode* right;

  BSTNode(uint16_t idx, const std::array<uint8_t, L>& cont)
      : index(idx), content(cont), left(nullptr), right(nullptr) {
  }

  void print() {
    std::cout << "Node " << index << "content: ";
    for (auto& c : content) {
      std::cout << c;
    }
    std::cout << std::endl;
  }
};

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

class BSTree {
  public:
  BSTNode* root;

  void destroy_recursive(BSTNode* node) {
    if (node) {
      destroy_recursive(node->left);
      destroy_recursive(node->right);
      delete node;
    }
  }

  public:
  void print_recursive_preorder(BSTNode* node) {
    if (node) {
      node->print();
      print_recursive_preorder(node->left);
      print_recursive_preorder(node->right);
    }
  }

  BSTree() : root(nullptr) {
    // return; // Not needed in void constructor
  }

  // Option 2: Insert by creating a node internally (Recommended)
  void insert_data(uint16_t index, const std::array<uint8_t, L>& content) {
    BSTNode* new_node = new BSTNode(index, content);  // Allocate node

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
        // Handle duplicates: if equal, go right.
        // If you want to disallow duplicates or update, add logic here.
        current_node = current_node->right;
      }
    }

    // Attach the new node
    if (compare_content_less(new_node->content, prev_node->content)) {
      prev_node->left = new_node;
    } else {
      prev_node->right = new_node;
    }
  }

  ~BSTree() {
    destroy_recursive(root);
  }

  // --- Other methods ---
  void remove_node() {
    // TODO: Implement removal
    return;
  }

  void find_longes_prefix() {
    // TODO: Implement prefix search
    return;
  }
};
