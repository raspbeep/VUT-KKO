#ifndef BLOCK_READER_HPP
#define BLOCK_READER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "token.hpp"

bool readTokensFromFileFunctional(const std::string& filename, uint16_t& width,
                                  uint16_t& height, uint16_t& offset_length,
                                  uint16_t& length_bits,
                                  std::vector<token_t>& tokens);

#endif