#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "block.hpp"
#include "token.hpp"

// Internal state for bit reading
static uint8_t reader_buffer = 0;
static int reader_bit_position = 8;  // Start as if buffer is empty
static bool reader_eof = false;

// Resets the internal state of the bit reader
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

  // Extract the bit
  bitValue = (reader_buffer >> (7 - reader_bit_position)) & 1;
  reader_bit_position++;

  return true;
}

bool read_bits_from_file(std::ifstream& file, int numBits, uint32_t& value) {
  if (numBits < 0 || numBits > 32) {
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  if (reader_eof)
    return false;

  value = 0;
  for (int i = 0; i < numBits; i++) {
    bool current_bit;
    if (!read_bit_from_file(file, current_bit)) {
      return false;
    }
    value = (value << 1) | (current_bit ? 1 : 0);
  }
  return true;
}

bool read_blocks_from_file(const std::string& filename, uint32_t& width,
                           uint32_t& height, uint16_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks) {
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
    // Read header not bit-packed for consistency
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

    if (adaptive) {
      uint32_t temp_block_size;
      if (!read_bits_from_file(file, 16, temp_block_size)) {
        throw std::runtime_error(
            "Failed to read block size for adaptive mode.");
      }
      // It's safer to check if the read value fits in uint16_t
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
        // Calculate the current block's width and height
        uint32_t current_block_width = width;
        uint32_t current_block_height = height;
        if (adaptive) {
          current_block_width =
              std::min<uint32_t>(BLOCK_SIZE, width - col * BLOCK_SIZE);
          current_block_height =
              std::min<uint32_t>(BLOCK_SIZE, height - row * BLOCK_SIZE);
        }

        uint64_t expected_decoded_bytes =
            static_cast<uint64_t>(current_block_width) * current_block_height;

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
        uint64_t total_decoded_bytes_for_block = 0;
        while (total_decoded_bytes_for_block < expected_decoded_bytes) {
          // read tokens for the block
          token_t token;
          bool flag_bit;
          // Read coded flag (1 bit)
          if (!read_bit_from_file(file, flag_bit)) {
            // EOF hit unexpectedly before the block was fully decoded
            std::cerr << "Warning: Unexpected EOF encountered while reading "
                         "token flag for block ("
                      << row << "," << col << ")."
                      << " Expected " << expected_decoded_bytes
                      << " bytes, got " << total_decoded_bytes_for_block << "."
                      << std::endl;
            goto end_reading;  // Exit loops
          }
          token.coded = flag_bit;

          // Read token data
          if (token.coded) {
            uint32_t temp_offset, temp_length;
            // Coded token: read offset and length
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
            // Increment decoded bytes count
            total_decoded_bytes_for_block +=
                (token.data.length + MIN_CODED_LEN);
          } else {
            uint32_t temp_value;
            // Uncoded token: read ASCII value (8 bits)
            if (!read_bits_from_file(file, 8, temp_value)) {
              std::cerr << "Warning: EOF encountered while reading value for "
                           "uncoded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.value = static_cast<uint8_t>(temp_value);
            // Increment decoded bytes count
            total_decoded_bytes_for_block++;
          }

          // If we successfully read all parts of the token, add it
          block.m_tokens[strategy].push_back(token);
        }  // End while (total_decoded_bytes_for_block < expected_decoded_bytes)

        // Sanity check after loop
        if (total_decoded_bytes_for_block != expected_decoded_bytes) {
          std::cerr << "Warning: Block (" << row << "," << col
                    << ") decoding finished, but byte count mismatch. Expected "
                    << expected_decoded_bytes << ", got "
                    << total_decoded_bytes_for_block << "." << std::endl;
          // This might happen if the last token caused an overshoot,
          // which indicates an encoding error or file corruption.
        }

        blocks.push_back(std::move(block));  // Use move for efficiency
      }  // End for col
    }  // End for row

  } catch (const std::exception& e) {
    std::cerr << "Error during file reading: " << e.what() << std::endl;
    reset_bit_reader_state();  // Reset state on error
    file.close();
    return false;
  }

end_reading:

  // Reset state after successful read or normal EOF break
  reset_bit_reader_state();
  file.close();

  // Check if file close failed? Optional.

  // Check if EOF was reached (expected) or if some other error occurred
  // Note: reader_eof might be true if the last read operation hit EOF exactly.
  // file.eof() might be more reliable after closing.
  // A more robust check might involve verifying the number of blocks read
  // matches expected.

  return true;
}

// Function to print tokens (same as before)
void printTokens(const std::vector<token_t>& tokens) {
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