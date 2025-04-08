#include <fstream>
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
    // Read the next byte if buffer is exhausted
    if (!file.read(reinterpret_cast<char*>(&reader_buffer), 1)) {
      // Check if EOF was reached *during* this read attempt
      reader_eof = file.eof();
      // If it wasn't EOF, it's some other read error (could throw here if
      // needed) For now, just signal failure if read failed for any reason
      return false;
    }
    reader_bit_position = 0;
  }

  // Extract the bit
  bitValue = (reader_buffer >> (7 - reader_bit_position)) & 1;
  reader_bit_position++;

  return true;  // Bit read successfully
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

// Function to read tokens from binary file with bit unpacking
bool read_blocks_from_file(const std::string& filename, uint16_t& width,
                           uint16_t& height, uint16_t& offset_length,
                           uint16_t& length_bits, std::vector<Block>& blocks) {
  blocks.clear();
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for reading: " << filename << std::endl;
    reset_bit_reader_state();
    return false;
  }

  reset_bit_reader_state();

  uint32_t dec_block_size;
  bool adaptive;

  try {
    // Read header (not bit-packed)
    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    file.read(reinterpret_cast<char*>(&height), sizeof(height));
    file.read(reinterpret_cast<char*>(&offset_length), sizeof(offset_length));
    file.read(reinterpret_cast<char*>(&length_bits), sizeof(length_bits));
    read_bit_from_file(file, adaptive);
    if (adaptive) {
      // file.read(reinterpret_cast<char*>(&dec_block_size),
      //           sizeof(dec_block_size));
      read_bits_from_file(file, 16, dec_block_size);
    }

    // TODO: fix this so it does make sense (should be > or < then max value
    // that can be stored in uint16_t)
    // // Basic validation after header read
    // if (offset_length > 16 || length_bits > 16) {
    //   throw std::runtime_error(
    //       "Header indicates offset/length bit size too large for uint16_t.");
    // }
    // if (offset_length == 0 || length_bits == 0) {
    //   // Allow 0 maybe? Depends on spec. Let's assume > 0 needed for coded
    //   // tokens. Consider adding a check later if needed.
    // }

    uint16_t n_row_blocks =
        adaptive ? (height + block_size - 1) / block_size : 1;
    uint16_t n_col_blocks =
        adaptive ? (width + block_size - 1) / block_size : 1;

    for (size_t row = 0; row < n_row_blocks; row++) {
      for (size_t col = 0; col < n_col_blocks; col++) {
        uint16_t block_width =
            adaptive ? std::min<uint16_t>(block_size, width - col * block_size)
                     : width;
        uint16_t block_height =
            adaptive ? std::min<uint16_t>(block_size, height - row * block_size)
                     : height;

        // read the strategy from the file
        uint32_t strategy = DEFAULT;
        read_bits_from_file(file, 2, strategy);

        if (strategy >= N_STRATEGIES) {
          std::cerr << "Error: Invalid strategy value read from file: "
                    << strategy << std::endl;
          reset_bit_reader_state();
          return false;
        }
        // create a new block and push it to the blocks vector
        Block block(block_width, block_height,
                    static_cast<SerializationStrategy>(strategy));

        // counter for the number of tokens read
        uint16_t total_decoded = 0;
        while (total_decoded != dec_block_size * dec_block_size) {
          // read tokens for the block
          token_t token;
          bool flag_bit;
          // Read coded flag (1 bit)
          if (!read_bit_from_file(file, flag_bit)) {
            // EOF hit exactly when trying to read the next token's flag. This
            // is a clean exit.
            break;
          }
          token.coded = flag_bit;

          // Read token data
          if (token.coded) {
            uint32_t temp_offset, temp_length;
            // Coded token: read offset and length
            if (!read_bits_from_file(file, offset_length, temp_offset)) {
              // EOF hit while reading offset bits. Incomplete token.
              std::cerr << "Warning: EOF encountered while reading offset for "
                           "coded token."
                        << std::endl;
              break;
            }
            token.data.offset = static_cast<uint16_t>(temp_offset);

            if (!read_bits_from_file(file, length_bits, temp_length)) {
              // EOF hit while reading length bits. Incomplete token.
              std::cerr << "Warning: EOF encountered while reading length for "
                           "coded token."
                        << std::endl;
              break;
            }
            token.data.length = static_cast<uint16_t>(temp_length);
            total_decoded += token.data.length + MIN_CODED_LEN;
          } else {
            uint32_t temp_value;
            // Uncoded token: read ASCII value (8 bits)
            if (!read_bits_from_file(file, 8, temp_value)) {
              // EOF hit while reading value bits. Incomplete token.
              std::cerr << "Warning: EOF encountered while reading value for "
                           "uncoded token."
                        << std::endl;
              break;
            }
            token.data.value = static_cast<uint8_t>(temp_value);
            total_decoded++;
          }

          // If we successfully read all parts of the token, add it
          block.m_tokens[strategy].push_back(token);
        }
        blocks.push_back(block);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error during file reading: " << e.what() << std::endl;
    reset_bit_reader_state();  // Reset state on error
    file.close();
    return false;
  }

  // Reset state after successful read or normal EOF break
  reset_bit_reader_state();
  file.close();
  // Check if file close failed? Optional.

  // Check if EOF was reached (expected) or if some other error occurred
  if (!reader_eof && !file.eof()) {
    std::cerr
        << "Warning: Reading finished, but EOF flag not set. Potential issue."
        << std::endl;
  }

  return true;
}

#include <iomanip>

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