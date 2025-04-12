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

#include <cstdint>  // Include necessary types
#include <string>   // Include necessary types
#include <vector>

#include "block.hpp"  // Include Block definition

/**
 * @brief Writes the compressed data, including header and tokenized blocks, to
 * a file using bit packing.
 * @param filename The path to the output file.
 * @param width The width of the original data.
 * @param height The height of the original data.
 * @param offset_bits The number of bits used for offsets in coded tokens.
 * @param length_bits The number of bits used for lengths in coded tokens.
 * @param adaptive Flag indicating if adaptive mode was used.
 * @param model Flag indicating if model preprocessing was used.
 * @param blocks A vector of Block objects containing the tokens to be written.
 * @return True if writing was successful, false otherwise (e.g., file error).
 */
bool write_blocks_to_stream(const std::string& filename, uint32_t width,
                            uint32_t height, uint16_t offset_bits,
                            uint16_t length_bits, bool adaptive, bool model,
                            const std::vector<Block>& blocks);

#endif  // BLOCK_WRITER_HPP
