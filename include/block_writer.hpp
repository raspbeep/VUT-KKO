#ifndef BLOCK_WRITER_HPP
#define BLOCK_WRITER_HPP

#include <vector>

#include "block.hpp"

bool write_blocks_to_stream(const std::string& filename, uint16_t width,
                            uint16_t height, uint16_t offset_length,
                            uint16_t length_bits, bool adaptive, bool model,
                            const std::vector<Block>& blocks);

#endif  // BLOCK_WRITER_PP
