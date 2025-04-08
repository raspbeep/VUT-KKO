#ifndef COMMON_HPP
#define COMMON_HPP

#include <cmath>
#include <cstdint>

#define DEBUG_DUMMY_SEQ 0
#define DEBUG_PRINT 0
#define DEBUG_COMP_ENC_UNENC 1

#define SEARCH_BUF_SIZE 63

// minimum encode length
#define MIN_CODED_LEN 3

#define N_BITS_CODED 10
// shift 1 to the left N_BITS_CODED times
// -1 to get the maximum value for N_BITS_CODED bits
// and add the minimum coded length to optimize for value mapping
constexpr uint16_t MAX_CODED_LEN = (1 << N_BITS_CODED) - 1 + MIN_CODED_LEN;

constexpr uint16_t constexpr_bits_needed(uint64_t n) {
  if (n <= 1) {
    return 0;
  }
  // Calculate bits needed for value n-1
  uint64_t max_val = n - 1;
  uint16_t bits = 0;
  while (max_val > 0) {
    max_val >>= 1;
    bits++;
  }
  return bits;
}

constexpr uint16_t OFFSET_BITS = constexpr_bits_needed(SEARCH_BUF_SIZE);  // 6
constexpr uint16_t LENGTH_BITS = constexpr_bits_needed(MAX_CODED_LEN);    // 11

const size_t TOKEN_CODED_LEN = 1 + OFFSET_BITS + LENGTH_BITS;  // 13
const size_t TOKEN_UNCODED_LEN = 1 + 8;                        // 8

struct StrategyResult {
  size_t n_coded_tokens;
  size_t n_unencoded_tokens;
};

const uint16_t block_size = 16;

constexpr size_t HORIZONTAL = 0;
constexpr size_t VERTICAL = 1;
constexpr size_t N_STRATEGIES = 2;
constexpr size_t DEFAULT = HORIZONTAL;

using SerializationStrategy = std::size_t;

#endif  // COMMON_HPP