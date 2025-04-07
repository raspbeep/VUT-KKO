#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <cstdint>
#include <vector>

// minimum encode length
#define MIN_CODED_LEN 3

#define N_BITS_CODED 4
// shift 1 to the left N_BITS_CODED times
// -1 to get the maximum value for N_BITS_CODED bits
// and add the minimum coded length to optimize for value mapping
constexpr uint16_t MAX_CODED_LEN = (1 << N_BITS_CODED) - 1 + MIN_CODED_LEN;

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
  struct HashNode {
    uint64_t position;  // position in the input stream
    HashNode* next;
  };

  HashNode** table;
  uint32_t size;

  private:
  uint32_t hash_function(uint8_t* content);

  uint16_t match_length(std::vector<uint8_t>& data, uint64_t current_pos,
                        HashNode* current);
};

#endif  // HASHTABLE_HPP
