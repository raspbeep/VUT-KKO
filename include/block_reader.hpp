/**
 * @file      block_reader.chp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the block reader implementation for bit packed
 * decompression output
 *
 * @date      12 April  2025 \n
 */

#ifndef BLOCK_READER_HPP
#define BLOCK_READER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "token.hpp"

bool read_blocks_from_file(const std::string& filename, uint32_t& width,
                           uint32_t& height, uint16_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks);

#endif