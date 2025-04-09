#ifndef BLOCK_READER_HPP
#define BLOCK_READER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "token.hpp"

bool read_blocks_from_file(const std::string& filename, uint16_t& width,
                           uint16_t& height, uint16_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks);

#endif