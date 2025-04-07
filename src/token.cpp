#include "token.hpp"

// Helper function to write bits to the stream
// Manages a buffer and writes full bytes when available
void write_bits(std::ofstream& ofs, uint64_t& buffer, uint8_t& buffer_bits_used,
                uint64_t value, int num_bits) {
  if (!ofs) {
    throw std::runtime_error("Output stream is not valid");
  }
  if (num_bits <= 0) {
    return;  // Nothing to write
  }
  if (num_bits > 64) {
    // Or handle differently if values larger than 64 bits are needed
    throw std::invalid_argument("Cannot write more than 64 bits at once");
  }

  // Mask value to ensure we only consider the lowest num_bits
  // Use 1ULL for unsigned long long literal
  if (num_bits < 64) {
    value &= (1ULL << num_bits) - 1;
  }
  // else: value uses all 64 bits, no mask needed

  while (num_bits > 0) {
    // How many bits can we fit into the current buffer?
    uint8_t space_in_buffer = 64 - buffer_bits_used;
    uint8_t bits_to_add = std::min((uint8_t)num_bits, space_in_buffer);

    // Extract the lowest 'bits_to_add' from the value
    uint64_t chunk = value & ((1ULL << bits_to_add) - 1);

    // Shift the chunk into the correct position in the buffer
    buffer |= (chunk << buffer_bits_used);

    // Update buffer state
    buffer_bits_used += bits_to_add;
    num_bits -= bits_to_add;
    value >>= bits_to_add;  // Remove the bits we just added

    // If buffer is full (64 bits), write 8 bytes to the file
    if (buffer_bits_used == 64) {
      // Write byte by byte (safer for endianness)
      for (int i = 0; i < 8; ++i) {
        char byte = static_cast<char>((buffer >> (i * 8)) & 0xFF);
        ofs.write(&byte, 1);
        if (!ofs) {
          throw std::runtime_error("Failed to write byte to file");
        }
      }
      buffer = 0;
      buffer_bits_used = 0;
    }
  }
}

// Function to flush any remaining bits in the buffer at the end
void flush_bits(std::ofstream& ofs, uint64_t& buffer,
                uint8_t& buffer_bits_used) {
  if (!ofs) {
    throw std::runtime_error("Output stream is not valid before flush");
  }
  if (buffer_bits_used > 0) {
    // Calculate how many full bytes are needed for the remaining bits
    uint8_t bytes_to_write = (buffer_bits_used + 7) / 8;  // Ceiling division

    // Write the necessary bytes
    for (uint8_t i = 0; i < bytes_to_write; ++i) {
      char byte = static_cast<char>((buffer >> (i * 8)) & 0xFF);
      ofs.write(&byte, 1);
      if (!ofs) {
        throw std::runtime_error("Failed to write final byte to file");
      }
    }
    buffer = 0;
    buffer_bits_used = 0;
  }
}

// Function to write the vector of tokens to a binary file
bool write_tokens_to_file(std::ofstream& ofs, std::vector<token_t>& tokens,
                          uint8_t N_OFFSET_BITS, uint8_t M_LENGTH_BITS) {
  if (N_OFFSET_BITS > 16 || M_LENGTH_BITS > 16) {
    std::cerr << "Error: N and M cannot exceed 16 bits (size of uint16_t)."
              << std::endl;
    return false;
  }
  if (N_OFFSET_BITS == 0 || M_LENGTH_BITS == 0) {
    std::cerr
        << "Warning: N or M is zero, corresponding data will not be written."
        << std::endl;
    // Allow proceeding, but warn the user.
  }

  // std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
  //   if (!ofs) {
  //     std::cerr << "Error: Cannot open file for writing: " << filename
  //               << std::endl;
  //     return false;
  //   }

  uint64_t bit_buffer = 0;
  uint8_t bits_in_buffer = 0;

  try {
    for (const auto& token : tokens) {
      // 1. Write the 'coded' bit (1 bit)
      write_bits(ofs, bit_buffer, bits_in_buffer, token.coded ? 1 : 0, 1);

      if (token.coded) {
        // 2a. Write 'offset' (N bits)
        write_bits(ofs, bit_buffer, bits_in_buffer, token.data.offset,
                   N_OFFSET_BITS);
        // 2b. Write 'length' (M bits)
        write_bits(ofs, bit_buffer, bits_in_buffer, token.data.length,
                   M_LENGTH_BITS);
      } else {
        // 2c. Write 'value' (8 bits)
        write_bits(ofs, bit_buffer, bits_in_buffer, token.data.value, 8);
      }
    }

    // 3. Flush any remaining bits from the buffer
    flush_bits(ofs, bit_buffer, bits_in_buffer);

  } catch (const std::exception& e) {
    std::cerr << "Error during file writing: " << e.what() << std::endl;
    ofs.close();  // Attempt to close even on error
    return false;
  }

  return !ofs.fail();  // Return true if close was successful and no errors
                       // occurred
}

// // --- Example Usage ---
// int test_writing() {
//   std::vector<token_t> my_tokens;

//   // Example: Literal token (coded = false)
//   token_t t1;
//   t1.coded = false;
//   t1.data.value = 'A';  // 0x41
//   my_tokens.push_back(t1);

//   // Example: Back-reference token (coded = true)
//   token_t t2;
//   t2.coded = true;
//   t2.data.offset = 1500;  // Example offset
//   t2.data.length = 25;    // Example length
//   my_tokens.push_back(t2);

//   token_t t3;
//   t3.coded = false;
//   t3.data.value = 'B';  // 0x42
//   my_tokens.push_back(t3);

//   // Define N and M (e.g., 12 bits for offset, 4 bits for length)
//   const uint8_t N = 12;
//   const uint8_t M = 4;

//   // Check if example data fits N and M
//   if (t2.data.offset >= (1 << N) || t2.data.length >= (1 << M)) {
//     std::cerr << "Warning: Example data exceeds chosen N/M bit limits."
//               << std::endl;
//     // The write_bits function will truncate, but it's good practice to
//     check.
//   }

//   std::string output_filename = "tokens.bin";
//   if (write_tokens_to_file(output_filename, my_tokens, N, M)) {
//     std::cout << "Successfully wrote tokens to " << output_filename
//               << std::endl;

//     // You would need a corresponding read function to verify
//     // The size will depend on N and M
//     // Token 1: 1b (coded=0) + 8b (value) = 9 bits
//     // Token 2: 1b (coded=1) + Nb (offset) + Mb (length) = 1 + 12 + 4 = 17
//     bits
//     // Token 3: 1b (coded=0) + 8b (value) = 9 bits
//     // Total = 9 + 17 + 9 = 35 bits
//     // Expected file size = ceil(35 / 8) = 5 bytes
//     std::ifstream ifs(output_filename, std::ios::binary | std::ios::ate);
//     if (ifs) {
//       std::cout << "Output file size: " << ifs.tellg() << " bytes."
//                 << std::endl;
//     }

//   } else {
//     std::cerr << "Failed to write tokens to file." << std::endl;
//   }

//   return 0;
// }