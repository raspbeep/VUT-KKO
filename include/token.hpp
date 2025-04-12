/**
 * @file      token.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file with token structure for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

/**
 * @struct token_t
 * @brief Represents a token in the LZSS compressed stream.
 *
 * A token can either represent a literal (uncoded) byte or a reference
 * (coded) to a previously seen sequence of bytes, defined by an offset
 * and length pair.
 */
typedef struct {
  bool coded;
  union {
    uint8_t value;
    struct {
      uint16_t offset;
      uint16_t length;
    };
  } data;

} token_t;

#endif  // TOKEN_HPP