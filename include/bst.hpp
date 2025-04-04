#ifndef BSTREE_HPP
#define BSTREE_HPP

#include <stdint.h>

#include <array>
#include <iostream>
#include <stdexcept>  // For potential exceptions if needed

#define L 5

// TODO: do I need this?
enum CompareContent {
  LESS,
  GREATER,
  EQUAL,
};

// Forward declaration of BSTNode
struct BSTNode {
  uint16_t index;
  std::array<uint8_t, L> content;
  BSTNode* left;
  BSTNode* right;

  // Constructor
  BSTNode(uint16_t idx, const std::array<uint8_t, L>& cont);

  // Print function
  void print();
};

// Function to compare two content arrays
bool compare_content_less(const std::array<uint8_t, L>& content_one,
                          const std::array<uint8_t, L>& content_two);

// Binary Search Tree class
class BSTree {
  public:
  BSTNode* root;

  // Helper function to recursively destroy nodes
  void destroy_recursive(BSTNode* node);

  public:
  // Constructor
  BSTree();

  // Destructor
  ~BSTree();

  // Insert a new node into the tree
  void insert_data(uint16_t index, const std::array<uint8_t, L>& content);

  // Print the tree in preorder traversal
  void print_recursive_preorder(BSTNode* node);

  // remove node by given index
  void remove_node(uint16_t index);

  void find_longes_prefix();
};

#endif  // BSTREE_HPP
