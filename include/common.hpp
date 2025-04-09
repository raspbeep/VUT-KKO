#ifndef COMMON_HPP
#define COMMON_HPP

#include <cmath>
#include <cstdint>

// debug priting flags
#define DEBUG_DUMMY_SEQ 0
#define DEBUG_PRINT 0
#define DEBUG_COMP_ENC_UNENC 0
#define DEBUG_PRINT_TOKENS 0

// minimum encode length
#define MIN_CODED_LEN 3

extern uint16_t SEARCH_BUF_SIZE;
extern uint16_t OFFSET_BITS;
extern uint16_t LENGTH_BITS;
extern uint16_t MAX_CODED_LEN;

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

// used only for statistics printing
extern size_t TOKEN_CODED_LEN;
extern size_t TOKEN_UNCODED_LEN;

struct StrategyResult {
  size_t n_coded_tokens;
  size_t n_unencoded_tokens;
};

constexpr uint16_t block_size = 16;

constexpr size_t HORIZONTAL = 0;
constexpr size_t VERTICAL = 1;
constexpr size_t N_STRATEGIES = 2;
constexpr size_t DEFAULT = HORIZONTAL;

using SerializationStrategy = std::size_t;

#endif  // COMMON_HPP