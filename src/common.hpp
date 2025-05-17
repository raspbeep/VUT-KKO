/**
 * @file      common.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Common header file for LZSS compression with configuration
 * parameters and default values
 *
 * @date      12 April  2025 \n
 */

#ifndef COMMON_HPP
#define COMMON_HPP

#include <cmath>
#include <cstdint>

// debug priting options
#define DEBUG_DUMMY_SEQ 0
#define DEBUG_PRINT 0
#define DEBUG_COMP_ENC_UNENC 0
#define DEBUG_PRINT_TOKENS 0
#define DEBUG_PRINT_COLLISIONS 0

#define BINARY_ONLY 1

// minimum encode length
#define MIN_CODED_LEN 3

#define DEFAULT_BLOCK_SIZE 512
#define DEFAULT_OFFSET_BITS 16
#define DEFAULT_LENGTH_BITS 9

// use MTF, if 0 use delta transform
#define MTF 1

extern uint16_t SEARCH_BUF_SIZE;
extern uint32_t OFFSET_BITS;
extern uint16_t LENGTH_BITS;
extern uint16_t MAX_CODED_LEN;

// used only for statistics printing
extern size_t TOKEN_CODED_LEN;
extern size_t TOKEN_UNCODED_LEN;

struct StrategyResult {
  size_t n_coded_tokens;
  size_t n_unencoded_tokens;
};

extern uint16_t BLOCK_SIZE;

constexpr size_t HORIZONTAL = 0;
constexpr size_t VERTICAL = 1;
constexpr size_t N_STRATEGIES = 2;
constexpr size_t DEFAULT = HORIZONTAL;

using SerializationStrategy = std::size_t;

#endif  // COMMON_HPP