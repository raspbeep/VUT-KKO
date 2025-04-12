/**
 * @file      block_writer.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the block writer implementation for bit packed
 * compression output
 *
 * @date      12 April  2025 \n
 */

#ifndef BLOCK_WRITER_HPP
#define BLOCK_WRITER_HPP

#include <vector>

#include "block.hpp"

bool write_blocks_to_stream(const std::string& filename, uint32_t width,
                            uint32_t height, uint16_t offset_length,
                            uint16_t length_bits, bool adaptive, bool model,
                            const std::vector<Block>& blocks);

#endif  // BLOCK_WRITER_PP
