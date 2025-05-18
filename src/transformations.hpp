/**
 * @file      transformations.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the transformation algorithms for LZSS
 * preprocessing
 *
 * @date      12 April  2025 \n
 */

#ifndef TRANSFORMATIONS_HPP
#define TRANSFORMATIONS_HPP

#include <cstdint>
#include <vector>

/**
 * @brief Applies Run-Length Encoding (RLE) with no explicit marker to the data.
 * @param data The input data vector, which will be modified in-place.
 */
void rle(std::vector<uint8_t>& data);

/**
 * @brief Reverses Run-Length Encoding (RLE) with no explicit marker on the
 * data.
 * @param data The RLE encoded data vector, which will be modified in-place.
 */
void reverse_rle(std::vector<uint8_t>& data);

/**
 * @brief Packs binary data (0x00 or 0xFF) into bits of bytes.
 * It also adjusts width, height, and expected size accordingly.
 * @param data The input data vector (containing 0x00 or 0xFF), modified
 * in-place.
 * @param m_width The width of the data, adjusted after packing.
 * @param m_height The height of the data, adjusted after packing.
 * @param expected_size The expected size of the data after packing.
 */
void binary_only_pack(std::vector<uint8_t>& data, uint32_t& m_width,
                      uint32_t& m_height, uint64_t& expected_size);

/**
 * @brief Unpacks data that was previously packed by binary_only_pack.
 * @param data The packed data vector, which will be modified in-place.
 */
void binary_only_unpack(std::vector<uint8_t>& data);

/**
 * @brief Applies Move-to-Front (MTF) transformation to the data.
 * @param data The input data vector, which will be modified in-place.
 */
void mtf_transform(std::vector<uint8_t>& data);

/**
 * @brief Reverses Move-to-Front (MTF) transformation on the data.
 * @param data The MTF transformed data vector, which will be modified in-place.
 */
void reverse_mtf_transform(std::vector<uint8_t>& data);

#endif  // TRANSFORMATIONS_HPP
