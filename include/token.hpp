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

// Function to write bits to a file
bool write_tokens_to_file(std::ofstream& ofs, std::vector<token_t>& tokens,
                          uint8_t N_OFFSET_BITS, uint8_t M_LENGTH_BITS);

#endif  // TOKEN_HPP