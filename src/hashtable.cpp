#include "hashtable.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

// shift value for rolling hash function
#define d 5

uint32_t HashTable::hash_function(uint8_t* content) {
  uint32_t key = 0;
  for (uint16_t i = 0; i < (MIN_CODED_LEN + 1); i++) {
    key = (key << d) ^ (content[i]);
    key %= size;
  }
  return key;
}

// allocates a hash table of size 'size' and initializes all entries to nullptr
HashTable::HashTable(uint32_t size) : size(size) {
  table = new HashNode*[size];
  for (uint16_t i = 0; i < size; ++i) {
    table[i] = nullptr;
  }
}

// iterates over the hash table and deletes all LL nodes and the hash table
// itself
HashTable::~HashTable() {
  for (uint16_t i = 0; i < size; ++i) {
    HashNode* current = table[i];
    while (current != nullptr) {
      HashNode* temp = current;
      current = current->next;
      delete temp;
    }
  }
  delete[] table;
}

// finds the index of the hash table for the given content and returns the
// longes prefix in the input values
// returns a struct with the following values:
// - found: true if a match was found
// - position: position in the input stream
// - length: length of the match
// if no match was found, returns a struct with found = false
search_result HashTable::search(std::vector<uint8_t>& data,
                                uint64_t current_pos) {
  uint32_t key = hash_function(&data[current_pos]);  // hashes M bytes of data

  HashNode* current = table[key];
  struct search_result result{
      false,  // found
      0,      // position
      0,      // length
  };

  // traverse the linked list at the index of the hash table
  while (current != nullptr) {
    bool match = true;
    // check the MIN_CODED_LEN bytes of data at the given position
    for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
      uint8_t cmp1 = data[current_pos + i];
      uint8_t cmp2 = data[current->position + i];
      if (cmp1 != cmp2) {
        // hash collision! hashes matched but the data is different
        current = current->next;
        match = false;
        break;
      }
    }
    if (!match) {
      continue;
    }
    uint16_t current_match_length = match_length(data, current_pos, current);
    if (current_match_length > result.length) {
      result.length = current_match_length;
      result.position = current->position;
      result.found = true;
    }
    current = current->next;
  }
  return result;
  // if no match was found, return the default result (found = false)
}

// finds the longest prefix in the input values
// returns the length of the longest prefix (up to MAX_CODED_LEN bytes)
uint16_t HashTable::match_length(std::vector<uint8_t>& data,
                                 uint64_t current_pos, HashNode* current) {
  uint16_t match_length = 0;
  for (uint16_t i = 0; i < MAX_CODED_LEN; ++i) {
    auto cmp1_index = current_pos + i + MIN_CODED_LEN;
    auto cmp2_index = current->position + i + MIN_CODED_LEN;
    auto cmp1 = data[cmp1_index];
    auto cmp2 = data[cmp2_index];
    if (cmp1 == cmp2) {
      match_length++;
    } else {
      break;
    }
  }
  // subtract the minimum coded length to get the actual length
  // of the prefix because the minimum coded length is guaranteed after hash
  // table lookup
  return match_length;
}

void HashTable::insert(std::vector<uint8_t>& data, uint64_t position) {
  HashNode* new_node = new HashNode;
  new_node->position = position;
  // hash of the MIN_CODED_LEN bytes of data at the given position
  uint32_t index = hash_function(&data[position]);

#if 0
  std::cout << "HashTable::insert: " << std::endl;
  std::cout << "  position: " << position << std::endl;
  std::cout << "  index: " << index << std::endl;
  std::cout << "  data: ";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<int>(data[position + i]) << " ";
  }
  std::cout << "(";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<char>(data[position + i]);
  }
  std::cout << ")" << std::endl;
  std::cout << std::endl;
#endif

  HashNode* current = table[index];
  if (current == nullptr) {
    table[index] = new_node;
    new_node->next = nullptr;
  } else {
    new_node->next = current;
    table[index] = new_node;
  }
}

void HashTable::remove(std::vector<uint8_t>& data, uint64_t position) {
  uint32_t key = hash_function(&data[position]);
  HashNode* current = table[key];
  HashNode* prev = nullptr;

  while (current != nullptr && current->position != position) {
    prev = current;
    current = current->next;
  }
#if 0
  std::cout << "HashTable::remove: " << std::endl;
  std::cout << "  position: " << position << std::endl;
  std::cout << "  index: " << key << std::endl;
  std::cout << "  data: ";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<int>(data[position + i]) << " ";
  }
  std::cout << "(";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<char>(data[position + i]);
  }
  std::cout << ")" << std::endl;
  std::cout << std::endl;
#endif

  if (current == nullptr) {
    // this should never happen
    throw std::runtime_error("Item not found in the hash table");
    return;
  }

  // we found the node matching the data
  if (prev == nullptr) {
    // delete the head and update the head pointer
    table[key] = current->next;
  } else {
    // delete the current node and update the previous node's next pointer
    prev->next = current->next;
  }

  // free the memory of the current node
  delete current;
}
