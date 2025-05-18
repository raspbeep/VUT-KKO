/**
 * @file      block_reader.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Block reader implementation for bit packed decompression output
 *
 * @date      12 April  2025 \n
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "block.hpp"
#include "token.hpp"

// internal state for bit reading
uint8_t reader_buffer = 0;
int reader_bit_position = 8;
bool reader_eof = false;

// resets the internal state of the bit reader
void reset_bit_reader_state() {
  reader_buffer = 0;
  reader_bit_position = 8;
  reader_eof = false;
}

bool read_bit_from_file(std::ifstream& file, bool& bitValue) {
  if (reader_eof) {
    return false;
  }

  if (reader_bit_position == 8) {
    if (!file.read(reinterpret_cast<char*>(&reader_buffer), 1)) {
      reader_eof = file.eof();
      return false;
    }
    reader_bit_position = 0;
  }

  // extract the bit
  bitValue = (reader_buffer >> (7 - reader_bit_position)) & 1;
  reader_bit_position++;

  return true;
}

bool read_bits_from_file(std::ifstream& file, int num_bits, uint32_t& value) {
  if (num_bits < 0 || num_bits > 32) {
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  if (reader_eof)
    return false;

  value = 0;
  for (int i = 0; i < num_bits; i++) {
    bool current_bit;
    if (!read_bit_from_file(file, current_bit)) {
      return false;
    }
    value = (value << 1) | (current_bit ? 1 : 0);
  }
  return true;
}

bool read_blocks_from_file(const std::string& filename, uint32_t& width,
                           uint32_t& height, uint32_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks, bool& binary_only) {
  blocks.clear();
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for reading: " << filename << std::endl;
    reset_bit_reader_state();
    return false;
  }

  reset_bit_reader_state();
  uint8_t successful_compression;
  try {
    // read header not bit-packed for consistency
    file.read(reinterpret_cast<char*>(&successful_compression),
              sizeof(successful_compression));
    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    file.read(reinterpret_cast<char*>(&height), sizeof(height));
    file.read(reinterpret_cast<char*>(&offset_bits), sizeof(offset_bits));
    file.read(reinterpret_cast<char*>(&length_bits), sizeof(length_bits));

    if (!file.good()) {
      throw std::runtime_error("Failed to read file header.");
    }

    if (!read_bit_from_file(file, model)) {
      throw std::runtime_error("Failed to read model flag.");
    }

    if (!read_bit_from_file(file, adaptive)) {
      throw std::runtime_error("Failed to read adaptive flag.");
    }

    if (!read_bit_from_file(file, binary_only)) {
      throw std::runtime_error("Failed to read adaptive flag.");
    }

    if (adaptive) {
      uint32_t temp_block_size;
      if (!read_bits_from_file(file, 16, temp_block_size)) {
        throw std::runtime_error(
            "Failed to read block size for adaptive mode.");
      }

      if (temp_block_size > UINT16_MAX) {
        throw std::runtime_error(
            "Block size read from file exceeds uint16_t max.");
      }
      BLOCK_SIZE = static_cast<uint16_t>(temp_block_size);
      if (BLOCK_SIZE == 0) {
        throw std::runtime_error("Adaptive mode read invalid block size (0).");
      }
    }

    uint16_t n_row_blocks = 1;
    uint16_t n_col_blocks = 1;
    if (adaptive) {
      if (BLOCK_SIZE == 0) {
        throw std::runtime_error(
            "Cannot calculate blocks: Adaptive mode but block size is zero.");
      }
      n_row_blocks = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
      n_col_blocks = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    for (uint16_t row = 0; row < n_row_blocks; row++) {
      for (uint16_t col = 0; col < n_col_blocks; col++) {
        // calculate the current block's width and height
        uint32_t current_block_width = width;
        uint32_t current_block_height = height;
        if (adaptive) {
          current_block_width =
              std::min<uint32_t>(BLOCK_SIZE, width - col * BLOCK_SIZE);
          current_block_height =
              std::min<uint32_t>(BLOCK_SIZE, height - row * BLOCK_SIZE);
        }

        // read the strategy from the file
        uint32_t strategy_val = DEFAULT;
        if (adaptive) {
          if (!read_bits_from_file(file, 2, strategy_val)) {
            std::cerr << "Warning: EOF or read error encountered while reading "
                         "strategy for block ("
                      << row << "," << col << ")." << std::endl;
            throw std::runtime_error(
                "Failed to read strategy for block. Possible EOF or read "
                "error.");
          }
        }

        uint32_t token_count = 0;
        read_bits_from_file(file, 32, token_count);

        if (strategy_val >= N_STRATEGIES) {
          std::cerr << "Error: Invalid strategy value read from file: "
                    << strategy_val << " for block (" << row << "," << col
                    << ")." << std::endl;
          reset_bit_reader_state();
          return false;
        }

        SerializationStrategy strategy =
            static_cast<SerializationStrategy>(strategy_val);

        Block block(current_block_width, current_block_height, strategy);
        for (uint32_t token_it = 0; token_it < token_count; token_it++) {
          // read tokens for the block
          token_t token;
          bool flag_bit;
          // read coded flag (1 bit)
          if (!read_bit_from_file(file, flag_bit)) {
            // EOF hit unexpectedly before the block was fully decoded
            goto end_reading;  // exit loops
          }
          token.coded = flag_bit;

          // read token data
          if (token.coded) {
            uint32_t temp_offset, temp_length;
            // roded token: read offset and length
            if (!read_bits_from_file(file, offset_bits, temp_offset)) {
              std::cerr << "Warning: EOF encountered while reading offset for "
                           "coded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.offset = static_cast<uint16_t>(temp_offset);

            if (!read_bits_from_file(file, length_bits, temp_length)) {
              std::cerr << "Warning: EOF encountered while reading length for "
                           "coded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.length = static_cast<uint16_t>(temp_length);
          } else {
            uint32_t temp_value;
            // uncoded token: read ASCII value (8 bits)
            if (!read_bits_from_file(file, 8, temp_value)) {
              std::cerr << "Warning: EOF encountered while reading value for "
                           "uncoded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.value = static_cast<uint8_t>(temp_value);
          }

          // if we successfully read all parts of the token, add it
          block.m_tokens[strategy].push_back(token);
        }

        blocks.push_back(std::move(block));
      }
    }

  } catch (const std::exception& e) {
    std::cerr << "Error during file reading: " << e.what() << std::endl;
    reset_bit_reader_state();  // reset state on error
    file.close();
    return false;
  }

end_reading:
  // reset state after successful read or normal EOF break
  reset_bit_reader_state();
  file.close();

  return true;
}

void print_tokens(const std::vector<token_t>& tokens) {
  std::cout << "Total tokens: " << tokens.size() << std::endl;
  std::cout << "------------------------------" << std::endl;

  for (size_t i = 0; i < tokens.size(); i++) {
    const auto& token = tokens[i];

    std::cout << "Token " << std::setw(3) << i << ": ";
    if (token.coded) {
      std::cout << "CODED   - Offset: " << std::setw(4) << token.data.offset
                << ", Length: " << std::setw(2) << token.data.length
                << std::endl;
    } else {
      char c = static_cast<char>(token.data.value);
      std::cout << "UNCODED - ASCII: " << std::setw(3)
                << static_cast<int>(token.data.value);
      if (token.data.value >= 32 && token.data.value <= 126) {
        std::cout << " ('" << c << "')";
      }
      std::cout << std::endl;
    }
  }
}