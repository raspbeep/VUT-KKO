#include "linkedlist.hpp"

#include <stdexcept>

void add_item(struct LLNode **node, uint64_t data) {
  struct LLNode *new_node = new LLNode;
  new_node->data = data;
  new_node->next = *node;
  *node = new_node;
}

void remove_item(struct LLNode **node, uint64_t data) {
  struct LLNode *current = *node;
  struct LLNode *prev = nullptr;

  while (current != nullptr && current->data != data) {
    prev = current;
    current = current->next;
  }

  if (current == nullptr) {
    // this should never happen
    throw std::runtime_error("Item not found in the list");
    return;
  }

  // we found the node matching the data
  if (prev == nullptr) {
    // delete the head and update the head pointer
    *node = current->next;
  } else {
    // delete the current node and update the previous node's next pointer
    prev->next = current->next;
  }

  // free the memory of the current node
  delete current;
}
