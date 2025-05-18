/**
 * @file      hashtable.hpp
 *
 * @author    Pavel Kratochvil \n
 * Faculty of Information Technology \n
 * Brno University of Technology \n
 * xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for hash table implementation for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <algorithm>  // Required for std::remove_if
#include <cstdint>
#include <vector>

#include "common.hpp"

// Default size for the hash table (power of 2 for efficient masking)
#define HASH_TABLE_SIZE (1024)

/**
 * @struct search_result
 * @brief Structure to hold the result of a hash table search.
 */
struct search_result {
  bool found;         // true if a match was found
  uint64_t position;  // position of the match start in the input stream
  uint16_t length;    // length of the match (beyond MIN_CODED_LEN)
};

/**
 * @class HashTable
 * @brief Implements a hash table using separate chaining (with vectors) to
 * store positions of byte sequences for LZSS dictionary matching.
 */
class HashTable {
  public:
  /**
   * @brief Constructs a HashTable with a specified size.
   * @param size The number of buckets in the hash table.
   */
  HashTable(uint32_t size);

  /**
   * @brief Destroys the HashTable, freeing allocated memory if necessary.
   */
  ~HashTable();

  /**
   * @brief Inserts the position of a byte sequence into the hash table.
   * Hashes the sequence starting at 'position' and adds a node to the
   * corresponding bucket's vector.
   * @param data The input data vector.
   * @param position The starting position of the sequence to insert.
   */
  void insert(std::vector<uint8_t>& data, uint64_t position);

  /**
   * @brief Removes the entry corresponding to a specific position from the hash
   * table. Used to maintain a sliding window.
   * @param data The input data vector (used for hashing).
   * @param position The starting position of the sequence to remove.
   * @throws std::runtime_error if the item to be removed is not found.
   */
  void remove(std::vector<uint8_t>& data, uint64_t position);

  /**
   * @brief Searches the hash table for the longest match for the sequence
   * starting at the current position.
   * @param data The input data vector.
   * @param current_pos The current position in the data vector to search from.
   * @return A search_result struct indicating if a match was found, its
   * position, and its length.
   */
  struct search_result search(std::vector<uint8_t>& data, uint64_t current_pos);

  private:
  /**
   * @struct HashNode
   * @brief Node structure for storing positions in the hash table buckets.
   */
  struct HashNode {
    uint64_t position;  // Position in the input stream
  };

  std::vector<std::vector<HashNode>>
      table;  // Each element is a vector of HashNodes for a bucket

  private:
  /**
   * @brief Calculates the hash index for a byte sequence starting at a given
   * position.
   * @param data The input data vector.
   * @param position The starting position of the sequence to hash.
   * @return The calculated hash table index.
   */
  inline uint32_t hash_function(std::vector<uint8_t>& data, uint64_t position);

  /**
   * @brief Calculates the length of the match between the sequence at
   * current_pos and the sequence starting at the position stored in a HashNode.
   * Assumes the first MIN_CODED_LEN bytes already match.
   * @param data The input data vector.
   * @param current_pos The current position in the data vector.
   * @param node_in_bucket The HashNode (from a bucket's vector) pointing to the
   * potential match position.
   * @return The length of the match beyond the initial MIN_CODED_LEN bytes.
   */
  uint16_t match_length(std::vector<uint8_t>& data, uint64_t current_pos,
                        const HashNode& node_in_bucket);
};

#endif  // HASHTABLE_HPP