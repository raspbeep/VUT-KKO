#ifndef TRANSFORMATIONS_HPP
#define TRANSFORMATIONS_HPP

#include <cstdint>
#include <vector>

void rle(std::vector<uint8_t>& data);
void reverse_rle(std::vector<uint8_t>& data);

void binary_only_pack(std::vector<uint8_t>& data, uint32_t& m_width,
                      uint32_t& m_height, uint64_t& expected_size);

void binary_only_unpack(std::vector<uint8_t>& data);

void mtf_transform(std::vector<uint8_t>& data);
void reverse_mtf_transform(std::vector<uint8_t>& data);

#endif