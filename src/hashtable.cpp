#include "hashtable.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

// shift value for rolling hash function
#define d 5

uint32_t fmix32(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

#if 1
const uint32_t TABLE_MASK = HASH_TABLE_SIZE - 1;

uint32_t HashTable::hash_function(std::vector<uint8_t>& data,
                                  uint64_t position) {
  // Basic check for sufficient data
  if (position + 3 > data.size()) {  // Assuming MIN_CODED_LEN is 3
    throw std::out_of_range(
        "Not enough data for hashing at the given position");
    // Or return 0; // Return 0 & TABLE_MASK
  }

  // Combine the 3 bytes into a uint32_t (Little-Endian)
  uint32_t k1 = 0;
  k1 |= static_cast<uint32_t>(data[position + 0]);
  k1 |= static_cast<uint32_t>(data[position + 1]) << 8;
  k1 |= static_cast<uint32_t>(data[position + 2]) << 16;

  // Simple Mixing (Knuth's multiplicative hash constant)
  k1 *= 0x9E3779B9;  // 2654435769 - A good prime multiplier
  k1 ^= k1 >> 16;    // Simple final mixing step

  // Use bitwise AND for power-of-2 table size
  return k1 & TABLE_MASK;
}
#else
uint32_t HashTable::hash_function(std::vector<uint8_t>& data,
                                  uint64_t position) {
  uint32_t key = 0;
  for (uint16_t i = 0; i < MIN_CODED_LEN; i++) {
    key |= static_cast<uint32_t>(data[position + i]);
    key <<= static_cast<uint32_t>(data[position + i]);
    key %= size;
  }
  return key;
}
#endif

// allocates a hash table of size 'size' and initializes all entries to nullptr
HashTable::HashTable(uint32_t size) : size(size), collision_count(0) {
  table = new HashNode*[size];
  for (uint16_t i = 0; i < size; ++i) {
    table[i] = nullptr;
  }
}

// iterates over the hash table and deletes all LL nodes and the hash table
// itself
HashTable::~HashTable() {
#if DEBUG_PRINT_COLLISIONS
  std::cout << "Collision count: " << collision_count << std::endl;
#endif
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
  uint32_t key = hash_function(data, current_pos);  // hashes M bytes of data

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
        collision_count++;
#if DEBUG_PRINT_COLLISIONS
        std::cout << "HashTable::search: hash collision!" << std::endl;
        std::cout << "string1: ";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<int>(data[current_pos + j]) << " ";
        }
        std::cout << "(";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<char>(data[current_pos + j]);
        }
        std::cout << ")" << std::endl;
        std::cout << "string2: ";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<int>(data[current->position + j]) << " ";
        }
        std::cout << "(";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<char>(data[current->position + j]);
        }
        std::cout << ")" << std::endl;
#endif
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

uint16_t HashTable::match_length(std::vector<uint8_t>& data,
                                 uint64_t current_pos, HashNode* current) {
  uint16_t effective_length_bits = std::min<uint16_t>(LENGTH_BITS, 16);
  uint16_t max_additional_length = (1U << effective_length_bits) - 1;

  uint16_t current_match_length = 0;
  for (uint16_t i = 0; i < max_additional_length; ++i) {
    auto cmp1_index = current_pos + MIN_CODED_LEN + i;
    auto cmp2_index = current->position + MIN_CODED_LEN + i;

    if (cmp1_index >= data.size() || cmp2_index >= data.size()) {
      break;
    }

    // Compare characters
    if (data[cmp1_index] == data[cmp2_index]) {
      current_match_length++;
    } else {
      break;
    }
  }

  return current_match_length;
}

void HashTable::insert(std::vector<uint8_t>& data, uint64_t position) {
  HashNode* new_node = new HashNode;
  new_node->position = position;
  // hash of the MIN_CODED_LEN bytes of data at the given position
  uint32_t index = hash_function(data, position);

#if DEBUG_PRINT
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
  uint32_t key = hash_function(data, position);
  HashNode* current = table[key];
  HashNode* prev = nullptr;

  while (current != nullptr && current->position != position) {
    prev = current;
    current = current->next;
  }
#if DEBUG_PRINT
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
