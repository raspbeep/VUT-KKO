#include "hashtable.hpp"

#include <array>
#include <stdexcept>
#include <vector>

// minimum encode length
#define MIN_CODED_LEN 3
// shift value for rolling hash function
#define d 5

#define N_BITS_CODED 4
// shift 1 to the left N_BITS_CODED times
// -1 to get the maximum value for N_BITS_CODED bits
// and add the minimum coded length to optimize for value mapping
constexpr uint16_t MAX_CODED_LEN = (1 << N_BITS_CODED) - 1 + MIN_CODED_LEN;

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
      -1,     // position
      0,      // length
  };

  // traverse the linked list at the index of the hash table
  while (current != nullptr) {
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
    if (data[current_pos + i] == data[current->position + i]) {
      match_length++;
    } else {
      break;
    }
  }
  // subtract the minimum coded length to get the actual length
  // of the prefix because the minimum coded length is guaranteed after hash
  // table lookup
  return match_length - MIN_CODED_LEN;
}

void HashTable::insert(uint32_t key, std::vector<uint8_t>& data,
                       uint64_t position) {
  HashNode* new_node = new HashNode;
  new_node->position = position;
  // hash of the MIN_CODED_LEN bytes of data at the given position
  uint32_t index = hash_function(&data[position]);

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
