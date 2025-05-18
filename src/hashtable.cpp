/**
 * @file      hashtable.cpp
 *
 * @author    Pavel Kratochvil \n
 * Faculty of Information Technology \n
 * Brno University of Technology \n
 * xkrato61@fit.vutbr.cz
 *
 * @brief     Hash table implementation for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#include "hashtable.hpp"

#include <algorithm>  // For std::remove_if, std::find_if
#include <array>
#include <iostream>
#include <stdexcept>

const uint32_t TABLE_MASK = HASH_TABLE_SIZE - 1;
uint16_t max_additional_length = (1U << LENGTH_BITS) - 1;

inline uint32_t HashTable::hash_function(std::vector<uint8_t>& data,
                                         uint64_t position) {
  uint32_t k1 = 0;
  uint64_t end_position = position + MIN_CODED_LEN > data.size()
                              ? data.size()
                              : position + MIN_CODED_LEN;
  uint16_t shift_left = 0;
  for (uint64_t i = position; i < end_position; i++) {
    k1 |= static_cast<uint32_t>(data[i]) << shift_left;
    shift_left += 8;
  }

  k1 *= 0x9E3779B9;
  k1 ^= k1 >> 16;

  return k1 & TABLE_MASK;
}

HashTable::HashTable(uint32_t size) : size(size), collision_count(0) {
  table.resize(size);
}

HashTable::~HashTable() {
#if DEBUG_PRINT_COLLISIONS
  std::cout << "Collision count: " << collision_count << std::endl;
#endif
}

search_result HashTable::search(std::vector<uint8_t>& data,
                                uint64_t current_pos) {
  uint32_t key = hash_function(data, current_pos);

  const auto& bucket = table[key];
  struct search_result result{
      false,
      0,
      0,
  };

  for (const HashNode& node_in_bucket : bucket) {
    bool match = true;
    // check if current_pos + MIN_CODED_LEN or node_in_bucket.position +
    // MIN_CODED_LEN would go out of bounds
    if (current_pos + MIN_CODED_LEN > data.size() ||
        node_in_bucket.position + MIN_CODED_LEN > data.size()) {
      continue;
    }

    for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
      // check the bounds read past the end of data for either string
      if (current_pos + i >= data.size() ||
          node_in_bucket.position + i >= data.size()) {
        match = false;
        break;
      }
      uint8_t cmp1 = data[current_pos + i];
      uint8_t cmp2 = data[node_in_bucket.position + i];
      if (__builtin_expect(cmp1 != cmp2, 0)) {
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
        match = false;
        break;
      }
    }
    if (__builtin_expect(!match, 0)) {
      continue;
    }
    uint16_t current_match_length =
        match_length(data, current_pos, node_in_bucket);
    if (current_match_length > result.length) {
      result.length = current_match_length;
      result.position = node_in_bucket.position;
      result.found = true;
    }
  }
  return result;
}

uint16_t HashTable::match_length(std::vector<uint8_t>& data,
                                 uint64_t current_pos,
                                 const HashNode& node_in_bucket) {
  uint16_t current_match_length = 0;
  for (uint16_t i = 0; i < max_additional_length; ++i) {
    auto cmp1_index = current_pos + MIN_CODED_LEN + i;
    auto cmp2_index = node_in_bucket.position + MIN_CODED_LEN + i;

    if (cmp1_index >= data.size() || cmp2_index >= data.size()) {
      break;
    }

    if (data[cmp1_index] == data[cmp2_index]) {
      current_match_length++;
    } else {
      break;
    }
  }
  return current_match_length;
}

void HashTable::insert(std::vector<uint8_t>& data, uint64_t position) {
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
  table[index].insert(table[index].end(), {position});
}

void HashTable::remove(std::vector<uint8_t>& data, uint64_t position) {
  uint32_t key = hash_function(data, position);
  auto& bucket = table[key];

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

  auto it_to_remove = std::find_if(
      bucket.end(), bucket.begin(),
      [position](const HashNode& node) { return node.position == position; });

  if (__builtin_expect(it_to_remove == bucket.end(), 0)) {
    throw std::runtime_error("Item not found in the hash table");
  }

  bucket.erase(it_to_remove);
}