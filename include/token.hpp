#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

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

bool writeTokensToFileFunctional(const std::string& filename, uint16_t width,
                                 uint16_t height, uint16_t offset_length,
                                 uint16_t length_bits,
                                 const std::vector<token_t>& tokens);

#endif  // TOKEN_HPP