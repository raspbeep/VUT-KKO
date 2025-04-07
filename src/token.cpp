#include "token.hpp"

// writer
// Internal state for bit writing
static uint8_t writer_buffer = 0;
static int writer_bit_count = 0;

// Resets the internal state of the bit writer
void resetBitWriterState() {
  writer_buffer = 0;
  writer_bit_count = 0;
}

// Writes a single bit to the file stream using static buffer
void writeBitToFile(std::ofstream& file, bool bit) {
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

// Writes multiple bits from a value
void writeBitsToFile(std::ofstream& file, uint32_t value, int numBits) {
  if (numBits < 0 || numBits > 32) {
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  for (int i = numBits - 1; i >= 0; i--) {
    writeBitToFile(file, (value >> i) & 1);
  }
}

// Flushes any remaining bits in the static buffer
void flushBitsToFile(std::ofstream& file) {
  if (!file.is_open() || !file.good()) {
    // Allow flushing even if file closed, just clear buffer state
    resetBitWriterState();
    return;
  }

  if (writer_bit_count > 0) {
    writer_buffer <<= (8 - writer_bit_count);  // Pad with zeros
    file.write(reinterpret_cast<const char*>(&writer_buffer), 1);
    if (!file.good()) {
      // Don't throw here, as flushing might happen during cleanup
      std::cerr << "Warning: Failed to write final byte during flush."
                << std::endl;
    }
  }
  // Reset state after flushing
  resetBitWriterState();
}

// --- Main Writing Logic ---

// Function to write tokens to binary file with bit packing
bool writeTokensToFileFunctional(const std::string& filename, uint16_t width,
                                 uint16_t height, uint16_t offset_length,
                                 uint16_t length_bits,
                                 const std::vector<token_t>& tokens) {
  std::ofstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for writing: " << filename << std::endl;
    return false;
  }

  // Reset static writer state before starting
  resetBitWriterState();

  try {
    // Write header (not bit-packed)
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    file.write(reinterpret_cast<const char*>(&offset_length),
               sizeof(offset_length));
    file.write(reinterpret_cast<const char*>(&length_bits),
               sizeof(length_bits));

    if (!file.good())
      throw std::runtime_error("Failed to write header.");

    // Write tokens with bit packing using functional helpers
    for (const auto& token : tokens) {
      // Write coded flag (1 bit)
      writeBitToFile(file, token.coded);

      // Write token data
      if (token.coded) {
        // Coded token: write offset and length with specified bit lengths
        if (offset_length > 16 || length_bits > 16) {
          throw std::out_of_range(
              "Offset/Length bit size too large for uint16_t.");
        }
        writeBitsToFile(file, token.data.offset, offset_length);
        writeBitsToFile(file, token.data.length, length_bits);
      } else {
        // Uncoded token: write ASCII value (8 bits)
        writeBitsToFile(file, token.data.value, 8);
      }
    }

    // Flush any remaining bits
    flushBitsToFile(file);  // Also resets state

  } catch (const std::exception& e) {
    std::cerr << "Error during file writing: " << e.what() << std::endl;
    // Ensure state is reset even on error before closing
    resetBitWriterState();
    file.close();
    return false;
  }

  file.close();
  std::cout << "File written successfully: " << filename << std::endl;
  return true;
}

// reader
// Internal state for bit reading
static uint8_t reader_buffer = 0;
static int reader_bit_position = 8;  // Start as if buffer is empty
static bool reader_eof = false;

// Resets the internal state of the bit reader
void resetBitReaderState() {
  reader_buffer = 0;
  reader_bit_position = 8;
  reader_eof = false;
}

// Reads a single bit from the file stream using static buffer
// Returns true if bit read successfully, false if EOF encountered
bool readBitFromFile(std::ifstream& file, bool& bitValue) {
  if (reader_eof) {
    return false;  // Already hit EOF, cannot read more
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

// Reads multiple bits into a value
// Returns true if all bits read successfully, false if EOF encountered mid-read
bool readBitsFromFile(std::ifstream& file, int numBits, uint32_t& value) {
  if (numBits < 0 || numBits > 32) {
    // Or return false, depending on desired error handling
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  if (reader_eof)
    return false;  // Cannot read if already at EOF

  value = 0;
  for (int i = 0; i < numBits; i++) {
    bool current_bit;
    if (!readBitFromFile(file, current_bit)) {
      // readBitFromFile returned false, meaning EOF was hit
      // reader_eof flag is already set internally by readBitFromFile
      return false;  // Signal that not all bits could be read
    }
    value = (value << 1) | (current_bit ? 1 : 0);
  }
  return true;  // All requested bits were read successfully
}

// --- Main Reading Logic ---

// Function to read tokens from binary file with bit unpacking
bool readTokensFromFileFunctional(
    const std::string& filename, uint16_t& width, uint16_t& height,
    uint16_t& offset_length, uint16_t& length_bits,
    std::vector<token_t>& tokens) {  // Pass tokens by ref
  tokens.clear();                    // Clear output vector
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for reading: " << filename << std::endl;
    resetBitReaderState();  // Reset state even if file open fails
    return false;
  }

  // Reset static reader state before starting
  resetBitReaderState();

  try {
    // Read header (not bit-packed)
    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    if (!file.good())
      throw std::runtime_error("Failed or incomplete read for width.");
    file.read(reinterpret_cast<char*>(&height), sizeof(height));
    if (!file.good())
      throw std::runtime_error("Failed or incomplete read for height.");
    file.read(reinterpret_cast<char*>(&offset_length), sizeof(offset_length));
    if (!file.good())
      throw std::runtime_error("Failed or incomplete read for offset_length.");
    file.read(reinterpret_cast<char*>(&length_bits), sizeof(length_bits));
    if (!file.good())
      throw std::runtime_error("Failed or incomplete read for length_bits.");

    // Basic validation after header read
    if (offset_length > 16 || length_bits > 16) {
      throw std::runtime_error(
          "Header indicates offset/length bit size too large for uint16_t.");
    }
    if (offset_length == 0 || length_bits == 0) {
      // Allow 0 maybe? Depends on spec. Let's assume > 0 needed for coded
      // tokens. Consider adding a check later if needed.
    }

    // Read tokens with bit unpacking using functional helpers
    while (true) {
      token_t token;
      bool flag_bit;

      // Read coded flag (1 bit)
      if (!readBitFromFile(file, flag_bit)) {
        // EOF hit exactly when trying to read the next token's flag. This is a
        // clean exit.
        break;
      }
      token.coded = flag_bit;

      // Read token data
      if (token.coded) {
        uint32_t temp_offset, temp_length;
        // Coded token: read offset and length
        if (!readBitsFromFile(file, offset_length, temp_offset)) {
          // EOF hit while reading offset bits. Incomplete token.
          std::cerr << "Warning: EOF encountered while reading offset for "
                       "coded token."
                    << std::endl;
          break;
        }
        token.data.offset = static_cast<uint16_t>(temp_offset);

        if (!readBitsFromFile(file, length_bits, temp_length)) {
          // EOF hit while reading length bits. Incomplete token.
          std::cerr << "Warning: EOF encountered while reading length for "
                       "coded token."
                    << std::endl;
          break;
        }
        token.data.length = static_cast<uint16_t>(temp_length);

      } else {
        uint32_t temp_value;
        // Uncoded token: read ASCII value (8 bits)
        if (!readBitsFromFile(file, 8, temp_value)) {
          // EOF hit while reading value bits. Incomplete token.
          std::cerr << "Warning: EOF encountered while reading value for "
                       "uncoded token."
                    << std::endl;
          break;
        }
        token.data.value = static_cast<uint8_t>(temp_value);
      }

      // If we successfully read all parts of the token, add it
      tokens.push_back(token);
    }

  } catch (const std::exception& e) {
    std::cerr << "Error during file reading: " << e.what() << std::endl;
    resetBitReaderState();  // Reset state on error
    file.close();
    return false;
  }

  // Reset state after successful read or normal EOF break
  resetBitReaderState();
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