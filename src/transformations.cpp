/**
 * @file      transformations.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Source file for the transformation algorithms for LZSS
 * preprocessing
 *
 * @date      12 April  2025 \n
 */

#include "transformations.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

void binary_only_pack(std::vector<uint8_t>& data, uint32_t& m_width,
                      uint32_t& m_height, uint64_t& expected_size) {
  // compress eights of bytes in m_data into one byte
  std::vector<uint8_t> compressed_data;
  compressed_data.reserve(data.size() / 8);
  for (size_t i = 0; i < data.size(); i += 8) {
    uint8_t packed_byte = 0;
    for (size_t j = 0; j < 8 && i + j < data.size(); ++j) {
      if (data[i + j] == 0xFF) {
        packed_byte |= (1 << (7 - j));
      }
    }
    compressed_data.push_back(packed_byte);
  }
  data.clear();
  data.shrink_to_fit();
  data = compressed_data;
  if (m_width == 1) {
    m_height = (m_height + 7) / 8;
    expected_size = m_height;
  } else {
    expected_size = (static_cast<uint64_t>(m_width) * m_height + 7) / 8;
    m_width = (m_width + 7) / 8;
  }
}

void binary_only_unpack(std::vector<uint8_t>& data) {
  std::vector<uint8_t> decompressed_data;
  bool finished_unpacking = false;

  for (uint8_t compressed_byte : data) {
    if (finished_unpacking)
      break;
    for (int j = 7; j >= 0; --j) {
      if ((compressed_byte >> j) & 1) {
        decompressed_data.push_back(0xFF);
      } else {
        decompressed_data.push_back(0x00);
      }
    }
  }

  data = decompressed_data;
}

void rle(std::vector<uint8_t>& data) {
  if (data.empty()) {
    return;
  }

  std::vector<uint8_t> encoded_data;
  encoded_data.reserve(data.size());
  size_t i = 0;

  while (i < data.size()) {
    uint8_t current = data[i];
    size_t count = 1;
    while (i + count < data.size() && data[i + count] == current &&
           count < 255 + 3) {
      count++;
    }

    if (count < 3) {
      for (size_t j = 0; j < count; j++) {
        encoded_data.push_back(current);
      }
    } else {
      encoded_data.push_back(current);
      encoded_data.push_back(current);
      encoded_data.push_back(current);
      encoded_data.push_back(static_cast<uint8_t>(count - 3));
    }
    i += count;
  }

  data.swap(encoded_data);
}

void reverse_rle(std::vector<uint8_t>& data) {
  if (data.empty()) {
    return;
  }

  std::vector<uint8_t> decoded_data;
  decoded_data.reserve(data.size() * 2);  // Conservative estimate
  size_t i = 0;

  while (i < data.size()) {
    if (i + 3 < data.size() && data[i] == data[i + 1] &&
        data[i + 1] == data[i + 2]) {
      uint8_t value = data[i];
      uint8_t count = data[i + 3];
      for (size_t j = 0; j < static_cast<size_t>(3) + count; j++) {
        decoded_data.push_back(value);
      }
      i += 4;
    } else {
      decoded_data.push_back(data[i]);
      i++;
    }
  }

  data.swap(decoded_data);
}

void mtf_transform(std::vector<uint8_t>& data) {
  std::vector<uint8_t> dictionary(256);
  std::iota(dictionary.begin(), dictionary.end(), 0);

  for (size_t i = 0; i < data.size(); ++i) {
    uint8_t current_byte = data[i];

    auto dict_it =
        std::find(dictionary.begin(), dictionary.end(), current_byte);

    if (dict_it == dictionary.end()) {
      throw std::logic_error(
          "MTF Error: Byte value not found in the 0-255 dictionary. "
          "Indicates a potential data corruption.");
    }

    uint8_t index =
        static_cast<uint8_t>(std::distance(dictionary.begin(), dict_it));

    data[i] = index;

    if (index != 0) {
      std::rotate(dictionary.begin(), dict_it, dict_it + 1);
    }
  }
}

void reverse_mtf_transform(std::vector<uint8_t>& data) {
  std::vector<uint8_t> dictionary(256);
  std::iota(dictionary.begin(), dictionary.end(), 0);

  for (size_t i = 0; i < data.size(); ++i) {
    uint8_t current_index = data[i];

    if (current_index >= dictionary.size()) {
      throw std::runtime_error(
          "MTF Decode Error: Invalid index encountered in encoded data.");
    }

    uint8_t decoded_byte = dictionary[current_index];

    data[i] = decoded_byte;
    if (current_index != 0) {
      auto dict_it = dictionary.begin() + current_index;

      std::rotate(dictionary.begin(), dict_it, dict_it + 1);
    }
  }
}