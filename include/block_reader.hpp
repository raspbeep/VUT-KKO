/**
 * @file      block_reader.hpp
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

#include "block.hpp"  // Include necessary header for Block class
#include "token.hpp"  // Include necessary header for token_t

/**
 * @brief Reads compressed data from a file, reconstructing header information
 * and tokenized blocks.
 * @param filename The path to the compressed input file.
 * @param width Output parameter for the width of the original data.
 * @param height Output parameter for the height of the original data.
 * @param offset_bits Output parameter for the number of bits used for offsets
 * in coded tokens.
 * @param length_bits Output parameter for the number of bits used for lengths
 * in coded tokens.
 * @param adaptive Output parameter indicating if adaptive mode was used during
 * compression.
 * @param model Output parameter indicating if model preprocessing was used
 * during compression.
 * @param blocks Output parameter, a vector to be filled with the reconstructed
 * Block objects containing tokens.
 * @return True if the file was read successfully and blocks were reconstructed,
 * false otherwise (e.g., file not found, read error, corrupted data).
 */
bool read_blocks_from_file(const std::string& filename, uint32_t& width,
                           uint32_t& height, uint16_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks);

#endif  // BLOCK_READER_HPP
