#include "block_writer.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "block.hpp"
#include "token.hpp"

static uint8_t writer_buffer = 0;
static int writer_bit_count = 0;

void reset_bit_writer_state() {
  writer_buffer = 0;
  writer_bit_count = 0;
}

void write_bit_to_file(std::ofstream& file, bool bit) {
  if (!file.is_open() || !file.good()) {
    throw std::runtime_error("File stream is not valid for writing.");
  }

  writer_buffer = (writer_buffer << 1) | (bit ? 1 : 0);
  writer_bit_count++;

  if (writer_bit_count == 8) {
    file.write(reinterpret_cast<const char*>(&writer_buffer), 1);
    if (!file.good()) {
      throw std::runtime_error("Failed to write byte to file.");
    }
    writer_buffer = 0;
    writer_bit_count = 0;
  }
}

void write_bits_to_file(std::ofstream& file, uint32_t value, int numBits) {
  if (numBits < 0 || numBits > 32) {
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  for (int i = numBits - 1; i >= 0; i--) {
    write_bit_to_file(file, (value >> i) & 1);
  }
}

void flush_bits_to_file(std::ofstream& file) {
  if (!file.is_open() || !file.good()) {
    reset_bit_writer_state();
    return;
  }

  if (writer_bit_count > 0) {
    writer_buffer <<= (8 - writer_bit_count);
    file.write(reinterpret_cast<const char*>(&writer_buffer), 1);
    if (!file.good()) {
      std::cerr << "Warning: Failed to write final byte during flush."
                << std::endl;
    }
  }
  // Reset state after flushing
  reset_bit_writer_state();
}

// Function to write tokens to binary file with bit packing
bool write_blocks_to_stream(const std::string& filename, uint16_t width,
                            uint16_t height, uint16_t offset_length,
                            uint16_t length_bits, bool adaptive, bool model,
                            const std::vector<Block>& blocks) {
  std::ofstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for writing: " << filename << std::endl;
    return false;
  }

  reset_bit_writer_state();
  uint8_t successful_compression = 1;
  try {
    file.write(reinterpret_cast<const char*>(&successful_compression),
               sizeof(successful_compression));
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    file.write(reinterpret_cast<const char*>(&offset_length),
               sizeof(offset_length));
    file.write(reinterpret_cast<const char*>(&length_bits),
               sizeof(length_bits));
    write_bit_to_file(file, model);
    write_bit_to_file(file, adaptive);
    if (adaptive) {
      std::cout << "Writing block size of " << BLOCK_SIZE << " to file."
                << std::endl;
      write_bits_to_file(file, BLOCK_SIZE, 16);
    }

    if (!file.good())
      throw std::runtime_error("Failed to write header.");

    for (const auto& block : blocks) {
      if (adaptive) {
        // write strategy as 2 bits
        write_bits_to_file(file, block.m_picked_strategy, 2);
      }

      // Write tokens with bit packing using functional helpers
      for (const auto& token : block.m_tokens[block.m_picked_strategy]) {
        // Write coded flag (1 bit)
        write_bit_to_file(file, token.coded);

        // Write token data
        if (token.coded) {
          // Coded token: write offset and length with specified bit lengths
          if (offset_length > 16 || length_bits > 16) {
            throw std::out_of_range(
                "Offset/Length bit size too large for uint16_t.");
          }
          write_bits_to_file(file, token.data.offset, offset_length);
          write_bits_to_file(file, token.data.length, length_bits);
        } else {
          // Uncoded token: write ASCII value (8 bits)
          write_bits_to_file(file, token.data.value, 8);
        }
      }
    }

    // Flush any remaining bits
    flush_bits_to_file(file);  // Also resets state

  } catch (const std::exception& e) {
    std::cerr << "Error during file writing: " << e.what() << std::endl;
    // Ensure state is reset even on error before closing
    reset_bit_writer_state();
    file.close();
    return false;
  }

  file.close();
  std::cout << "File written successfully: " << filename << std::endl;
  return true;
}