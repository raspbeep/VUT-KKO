#include <assert.h>
#include <math.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "argparser.hpp"
#include "block.hpp"
#include "block_reader.hpp"
#include "block_writer.hpp"
#include "common.hpp"
#include "hashtable.hpp"
#include "token.hpp"

uint16_t BLOCK_SIZE = DEFAULT_BLOCK_SIZE;

// coded token parameters
uint16_t OFFSET_BITS = DEFAULT_OFFSET_BITS;
uint16_t LENGTH_BITS = DEFAULT_LENGTH_BITS;

// max number expressible with the OFFSET_LENGTH bits
uint16_t SEARCH_BUF_SIZE = (1 << OFFSET_BITS) - 1;

// designates the max length in longest prefix searching
// finds the maximum value we can represent with the given number of bits
// and add the minimum coded length to optimize for value mapping
uint16_t MAX_CODED_LEN = (1 << LENGTH_BITS) - 1 + MIN_CODED_LEN;

// used only for statistics printing
size_t TOKEN_CODED_LEN = 1 + OFFSET_BITS + LENGTH_BITS;
size_t TOKEN_UNCODED_LEN = 1 + 8;

class Image {
  public:
  // Constructor for encoding
  Image(std::string i_filename, std::string o_filename, uint16_t width,
        bool adaptive, bool model)
      : m_input_filename(i_filename),
        m_output_filename(o_filename),
        m_width(width),
        m_adaptive(adaptive),
        m_model(model) {
    // read the input file and store it in m_data vector
    read_enc_input_file();
    if (m_data.size() != static_cast<size_t>(m_width) * m_height) {
      throw std::runtime_error(
          "Error: Data size does not match image dimensions.");
    }
  }

  // Constructor for decoding
  Image(std::string i_filename, std::string o_filename)
      : m_input_filename(i_filename), m_output_filename(o_filename) {
    // read the input file and store it in m_data vector

    read_dec_input_file();
  }

  ~Image() {
    if (o_file_handle.is_open()) {
      o_file_handle.close();
    }
  }

  void read_dec_input_file() {
    // read the input file and store the tokens in m_tokens vector
    // store all the params from header in the class variables

    read_blocks_from_file(m_input_filename, m_width, m_height, OFFSET_BITS,
                          LENGTH_BITS, m_adaptive, m_model, m_blocks);
  }

  void read_enc_input_file() {
    std::ifstream file(m_input_filename, std::ios::binary);
    if (!file) {
      throw std::runtime_error("Error: Unable to open input file: " +
                               m_input_filename);
    }

    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0, std::ios::beg);

    m_height = length / m_width;

    uint64_t expected_size = static_cast<uint64_t>(m_width) * m_height;
    if (length < 0 || static_cast<uint64_t>(length) != expected_size) {
      std::ostringstream error_msg;
      error_msg << "Error: Input file '" << m_input_filename
                << "' size mismatch. Expected " << expected_size << " bytes ("
                << m_width << "x" << m_height << "), but got " << length
                << " bytes.";
      throw std::runtime_error(error_msg.str());
    }

    m_data.reserve(expected_size);
    m_data.assign((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());

    if (m_data.size() != expected_size) {
      std::ostringstream error_msg;
      error_msg << "Error: Read incomplete data from file '" << m_input_filename
                << "'. Expected " << expected_size << " bytes, read "
                << m_data.size() << " bytes.";
      throw std::runtime_error(error_msg.str());
    }
  }

  void write_dec_output_file() {
    // write the decoded data to the output file
    std::ofstream o_file_handle(m_output_filename, std::ios::binary);
    if (!o_file_handle) {
      throw std::runtime_error("Error: Unable to open output file: " +
                               m_output_filename);
    }
    o_file_handle.write(reinterpret_cast<const char*>(m_data.data()),
                        m_data.size());
    o_file_handle.close();
    std::cout << "Decoded data written to: " << m_output_filename << std::endl;
    std::cout << "Written " << m_data.size() << " bytes." << std::endl;
  }

  void create_blocks() {
    if (!m_adaptive) {
      create_single_block();
    } else {
      create_multiple_blocks();
    }
  }

  void print_blocks() {
    for (size_t i = 0; i < m_blocks.size(); i++) {
      Block& block = m_blocks[i];
      std::cout << "Block #" << i << " of width: " << block.m_width
                << " and height: " << block.m_height << std::endl;
    }
  }

  void encode_blocks() {
    std::vector<token_t> tokens;
    // iterate over all blocks, serialize, encode and write them
    for (size_t i = 0; i < m_blocks.size(); i++) {
      Block& block = m_blocks[i];
      if (m_adaptive) {
        block.serialize_all_strategies();
        if (m_model)
          for (size_t j = 0; j < N_STRATEGIES; j++) {
            block.delta_transform(static_cast<SerializationStrategy>(j));
          }
        block.encode_adaptive();
      } else {
        if (m_model)
          block.delta_transform(DEFAULT);
        block.encode_using_strategy(DEFAULT);
      }
#if DEBUG_PRINT
      std::cout << "Block #" << i
                << " picked strategy: " << block.m_picked_strategy << std::endl;
#endif
#if DEBUG_COMP_ENC_UNENC
      block.decode_using_strategy(DEFAULT);
      block.compare_encoded_decoded();
#endif
#if DEBUG_PRINT_TOKENS
      block.print_tokens();
#endif
    }

    // we will not need the m_data anymore
    m_data.clear();
  }

  void write_blocks() {
    write_blocks_to_stream(m_output_filename, m_width, m_height, OFFSET_BITS,
                           LENGTH_BITS, m_adaptive, m_model, m_blocks);
  }

  void decode_blocks() {
    for (size_t i = 0; i < m_blocks.size(); i++) {
      Block& block = m_blocks[i];
      block.decode_using_strategy(DEFAULT);
#if DEBUG_PRINT_TOKENS
      block.print_tokens();
#endif
      if (m_model) {
        block.reverse_delta_transform();
      }
      if (m_adaptive) {
        block.deserialize();
      }
    }
  }

  void compose_image() {
    if (m_blocks.empty()) {
      std::cerr << "Warning: No blocks available to compose the image."
                << std::endl;
      m_data.clear();
      return;
    }

    uint64_t expected_size = static_cast<uint64_t>(m_width) * m_height;

    m_data.clear();

    if (expected_size != 0) {
      m_data.resize(expected_size);
    }

    if (!m_adaptive) {
      if (m_blocks.size() != 1) {
        throw std::runtime_error(
            "Error composing image: Non-adaptive mode expects exactly one "
            "block, found " +
            std::to_string(m_blocks.size()));
      }
      const Block& single_block = m_blocks[0];

      if (single_block.m_width != m_width ||
          single_block.m_height != m_height) {
        throw std::runtime_error(
            "Error composing image: Single block dimensions mismatch image "
            "dimensions.");
      }
      if (single_block.m_decoded_data.size() != expected_size) {
        throw std::runtime_error(
            "Error composing image: Decoded data size mismatch in "
            "non-adaptive mode.");
      }

      m_data = single_block.m_decoded_data;

    } else {
      uint16_t n_blocks_rows = (m_height + BLOCK_SIZE - 1) / BLOCK_SIZE;
      uint16_t n_blocks_cols = (m_width + BLOCK_SIZE - 1) / BLOCK_SIZE;
      size_t block_index = 0;

      // Iterate Row by Row, then Column by Column
      for (uint16_t block_r = 0; block_r < n_blocks_rows; ++block_r) {
        uint16_t start_row = block_r * BLOCK_SIZE;  // Correct row offset

        for (uint16_t block_c = 0; block_c < n_blocks_cols; ++block_c) {
          uint16_t start_col = block_c * BLOCK_SIZE;  // Correct col offset

          if (block_index >= m_blocks.size()) {
            throw std::runtime_error(
                "Error composing image: Not enough blocks provided for image "
                "dimensions.");
          }

          Block& current_block = m_blocks[block_index];
          // Ensure you get the final deserialized data if applicable
          const std::vector<uint8_t>& block_data =
              current_block.get_decoded_data();
          uint16_t current_block_height = current_block.m_height;
          uint16_t current_block_width = current_block.m_width;

          // ... (check block_data size against block dimensions) ...

          size_t block_data_idx = 0;
          // Place data into the final image using correct destination indices
          for (uint16_t r = 0; r < current_block_height; ++r) {
            for (uint16_t c = 0; c < current_block_width; ++c) {
              size_t dest_row = start_row + r;
              size_t dest_col = start_col + c;

              // Use correct image dimensions for bounds check
              if (dest_row >= m_height || dest_col >= m_width) {
                throw std::runtime_error(
                    "Error composing image: Calculated destination index out "
                    "of image bounds.");
              }

              // Index into final image data
              size_t dest_index = dest_row * m_width + dest_col;
              if (dest_index >= m_data.size()) {
                throw std::runtime_error(
                    "Error: Calculated destination index out of bounds during "
                    "composition.");
              }
              if (block_data_idx >= block_data.size()) {
                throw std::runtime_error(
                    "Error: Source block data index out of bounds during "
                    "composition.");
              }

              m_data[dest_index] = block_data[block_data_idx++];
            }
          }
          block_index++;
        }
      }

      if (block_index != m_blocks.size()) {
        std::cerr << "Warning: Composed image using " << block_index
                  << " blocks, but " << m_blocks.size()
                  << " were decoded. File might be incomplete." << std::endl;
      }
    }

    if (m_data.size() != expected_size) {
      throw std::runtime_error(
          "Error composing image: Final composed image size (" +
          std::to_string(m_data.size()) + ") does not match expected size (" +
          std::to_string(expected_size) + ").");
    }
  }

  uint16_t get_width() const {
    return m_width;
  }

  uint16_t get_height() const {
    return m_height;
  }

  bool is_adaptive() const {
    return m_adaptive;
  }

  bool is_compression_successful() {
    size_t coded = 0;
    size_t uncoded = 0;
    for (auto& block : m_blocks) {
      auto strategy = block.m_picked_strategy;
      for (auto& token : block.m_tokens[strategy]) {
        token.coded ? coded++ : uncoded++;
      }
    }
    size_t file_header_bits =
        4 * 16 + 1 + 1;  // width, height, offset, length, model, adaptive
    if (m_adaptive) {
      file_header_bits += 16;
    }

    size_t total_token_bits =
        (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded);

    size_t total_strategy_bits = m_blocks.size() * 2;
    size_t total_size_bits =
        file_header_bits + total_token_bits + total_strategy_bits;

    size_t size_original = static_cast<size_t>(m_width) * m_height;

    size_t total_size_bytes = static_cast<size_t>(ceil(total_size_bits / 8.0));

    std::cout << "--- Compression Stats ---" << std::endl;
    std::cout << "Original Size: " << size_original << " bytes" << std::endl;
    std::cout << "Compressed Size: " << total_size_bytes << " bytes"
              << std::endl;
    return total_size_bytes < size_original;
  }

  void reverse_transform() {
    for (auto& block : m_blocks) {
      block.reverse_delta_transform();
    }
  }

  private:
  void create_single_block() {
    // single block
    m_blocks.reserve(1);
    m_blocks.push_back(Block(m_data, m_width, m_width));
  }

  void create_multiple_blocks() {
    m_blocks.clear();
    // Correctly calculate row/column block counts
    uint16_t n_blocks_rows = (m_height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint16_t n_blocks_cols = (m_width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    m_blocks.reserve(static_cast<size_t>(n_blocks_rows) * n_blocks_cols);

    // Iterate Row by Row, then Column by Column (more conventional)
    for (uint16_t block_r = 0; block_r < n_blocks_rows; ++block_r) {
      // Calculate row offset based on row index
      uint16_t start_row = block_r * BLOCK_SIZE;
      // Correctly calculate block height using m_height
      uint16_t current_block_height =
          std::min<uint16_t>(BLOCK_SIZE, m_height - start_row);

      for (uint16_t block_c = 0; block_c < n_blocks_cols; ++block_c) {
        // Calculate column offset based on column index
        uint16_t start_col = block_c * BLOCK_SIZE;
        // Correctly calculate block width using m_width
        uint16_t current_block_width =
            std::min<uint16_t>(BLOCK_SIZE, m_width - start_col);

        std::vector<uint8_t> block_data;
        block_data.reserve(static_cast<size_t>(current_block_width) *
                           current_block_height);

        // Extract data using correct dimensions and offsets
        for (uint16_t r = start_row; r < start_row + current_block_height;
             ++r) {
          for (uint16_t c = start_col; c < start_col + current_block_width;
               ++c) {
            // Index into original image data
            size_t index = static_cast<size_t>(r) * m_width + c;
            if (index >= m_data.size()) {
              // Add robust error handling just in case
              throw std::runtime_error(
                  "Error: Calculated index out of bounds during block "
                  "creation.");
            }
            block_data.push_back(m_data[index]);
          }
        }

        // Verify extracted data size (optional but good practice)
        if (block_data.size() !=
            static_cast<size_t>(current_block_width) * current_block_height) {
          std::cerr << "Error: Internal logic error during block creation. "
                       "Size mismatch."
                    << " Expected: "
                    << (static_cast<size_t>(current_block_width) *
                        current_block_height)
                    << " Got: " << block_data.size() << std::endl;
          throw std::runtime_error("Block data size mismatch during creation.");
        }

        // Create block with correct data and dimensions
        m_blocks.emplace_back(std::move(block_data), current_block_width,
                              current_block_height);
      }
    }
    // Verify total number of blocks created (optional)
    if (m_blocks.size() != static_cast<size_t>(n_blocks_rows) * n_blocks_cols) {
      std::cerr << "Warning: Number of created blocks (" << m_blocks.size()
                << ") does not match expected count ("
                << (static_cast<size_t>(n_blocks_rows) * n_blocks_cols) << ")."
                << std::endl;
    }
  }

  private:
  std::string m_input_filename;
  std::string m_output_filename;
  std::ofstream o_file_handle;
  uint16_t m_width;
  uint16_t m_height;
  bool m_adaptive;
  bool m_model;
  std::vector<uint8_t> m_data;
  std::vector<token_t> m_tokens;

  public:
  std::vector<Block> m_blocks;
};

void print_final_stats(Image& img) {
  size_t coded = 0;
  size_t uncoded = 0;
  for (auto& block : img.m_blocks) {
    auto strategy = block.m_picked_strategy;
    for (auto& token : block.m_tokens[strategy]) {
      token.coded ? coded++ : uncoded++;
    }
  }
  size_t file_header_bits =
      4 * 16 + 1 + 1;  // width, height, offset, length, model, adaptive
  if (img.is_adaptive()) {
    file_header_bits += 16;
  }

  size_t total_token_bits =
      (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded);

  size_t total_strategy_bits = img.m_blocks.size() * 2;
  size_t total_size_bits =
      file_header_bits + total_token_bits + total_strategy_bits;

  size_t size_original =
      static_cast<size_t>(img.get_width()) * img.get_height() * 8;
  double compression_ratio =
      (total_size_bits > 0)
          ? (static_cast<double>(size_original) / total_size_bits)
          : 0.0;
  size_t total_size_bytes = static_cast<size_t>(ceil(total_size_bits / 8.0));

  std::cout << "--- Final Stats ---" << std::endl;
  std::cout << "Image Dimensions: " << img.get_width() << "x"
            << img.get_height() << std::endl;
  std::cout << "Adaptive Mode: " << (img.is_adaptive() ? "Yes" : "No")
            << std::endl;
  if (img.is_adaptive()) {
    std::cout << "Block Size: " << BLOCK_SIZE << "x" << BLOCK_SIZE << std::endl;
  }
  std::cout << "Number of Blocks: " << img.m_blocks.size() << std::endl;
  std::cout << "Offset Bits: " << OFFSET_BITS
            << ", Length Bits: " << LENGTH_BITS << std::endl;
  std::cout << "Original data size: " << size_original << "b ("
            << size_original / 8 << "B)" << std::endl;
  std::cout << "Coded tokens: " << coded << " (" << TOKEN_CODED_LEN * coded
            << "b)" << std::endl;
  std::cout << "Uncoded tokens: " << uncoded << " ("
            << TOKEN_UNCODED_LEN * uncoded << "b)" << std::endl;
  std::cout << "File Header Size: " << file_header_bits << "b" << std::endl;
  std::cout << "Block Strategy Headers Size: " << total_strategy_bits << "b"
            << std::endl;
  std::cout << "Total Token Data Size: " << total_token_bits << "b"
            << std::endl;
  std::cout << "Calculated Total Size: " << total_size_bits << "b ("
            << total_size_bytes << "B)" << std::endl;
  std::cout << "Compression Ratio (Original Bits / Total Bits): "
            << compression_ratio << std::endl;
  double space_saved_percent =
      (size_original > 0)
          ? (1.0 - (static_cast<double>(total_size_bits) / size_original)) *
                100.0
          : 0.0;

  std::cout << "Space Saved: " << std::fixed << std::setprecision(2)
            << space_saved_percent << "%" << std::endl;
}

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);

  assert(OFFSET_BITS <= 15);
  assert(LENGTH_BITS <= 15);

  if (args.is_compress_mode()) {
    Image i =
        Image(args.get_input_file(), args.get_output_file(),
              args.get_image_width(), args.is_adaptive(), args.use_model());
    i.create_blocks();
    i.encode_blocks();
    // if (i.is_compression_successful()) {
    //   std::cout << "Compression successful. Writing to: "
    //             << args.get_output_file() << std::endl;
    i.write_blocks();
    // } else {
    //   // copy original file
    //   std::ifstream src(args.get_input_file(), std::ios::binary);
    //   std::ofstream dst(args.get_output_file(), std::ios::binary);
    //   dst << src.rdbuf();
    //   src.close();
    //   dst.close();
    //   std::cout << "Compression not successful. Original file copied to: "
    //             << args.get_output_file() << std::endl;
    // }
    // print_final_stats(i);
  } else {
    Image i = Image(args.get_input_file(), args.get_output_file());
    i.decode_blocks();
    i.compose_image();
    i.write_dec_output_file();
  }

  return 0;
}