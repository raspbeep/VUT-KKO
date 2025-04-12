#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <cstdint>
#include <vector>

#include "common.hpp"

#define HASH_TABLE_SIZE (1 << 12)

struct search_result {
  bool found;         // true if a match was found
  uint64_t position;  // position in the input stream
  uint16_t length;    // length of the match
};

class HashTable {
  public:
  HashTable(uint32_t size);
  ~HashTable();

  void insert(std::vector<uint8_t>& data, uint64_t position);

  void remove(std::vector<uint8_t>& data, uint64_t position);

  struct search_result search(std::vector<uint8_t>& data, uint64_t current_pos);

  private:
  // linked list structure for hash table
  struct HashNode {
    uint64_t position;  // position in the input stream
    HashNode* next;
  };

  HashNode** table;
  uint32_t size;
  uint64_t collision_count;

  private:
  uint32_t hash_function(std::vector<uint8_t>& data, uint64_t position);

  uint16_t match_length(std::vector<uint8_t>& data, uint64_t current_pos,
                        HashNode* current);
};

#endif  // HASHTABLE_HPP
