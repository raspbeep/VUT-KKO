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
#include "bst.hpp"
#include "common.hpp"
#include "hashtable.hpp"
#include "token.hpp"

// coded token parameters
uint16_t OFFSET_BITS = 10;
uint16_t LENGTH_BITS = 8;

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
    if (m_data.size() != static_cast<size_t>(m_width) * m_width) {
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

    read_blocks_from_file(m_input_filename, m_width, m_width, OFFSET_BITS,
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

    uint64_t expected_size = static_cast<uint64_t>(m_width) * m_width;
    if (length < 0 || static_cast<uint64_t>(length) != expected_size) {
      std::ostringstream error_msg;
      error_msg << "Error: Input file '" << m_input_filename
                << "' size mismatch. Expected " << expected_size << " bytes ("
                << m_width << "x" << m_width << "), but got " << length
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
      // block.delta_transform(m_adaptive);
      if (m_adaptive) {
        block.serialize_all_strategies();
        block.encode_adaptive();
      } else {
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

    write_blocks_to_stream(m_output_filename, m_width, m_width, OFFSET_BITS,
                           LENGTH_BITS, m_adaptive, m_model, m_blocks);
  }

  void decode_blocks() {
    for (size_t i = 0; i < m_blocks.size(); i++) {
      Block& block = m_blocks[i];
      block.decode_using_strategy(DEFAULT);
#if DEBUG_PRINT_TOKENS
      block.print_tokens();
#endif
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

    uint64_t expected_size = static_cast<uint64_t>(m_width) * m_width;

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

      if (single_block.m_width != m_width || single_block.m_height != m_width) {
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
      uint16_t n_blocks_dim = (m_width + block_size - 1) / block_size;
      size_t block_index = 0;  // To iterate through the m_blocks vector

      // Iterate through the grid of blocks (row by row, then column by column)
      for (uint16_t block_r = 0; block_r < n_blocks_dim; ++block_r) {
        uint16_t start_row =
            block_r *
            block_size;  // Top row of the current block in the final image

        for (uint16_t block_c = 0; block_c < n_blocks_dim; ++block_c) {
          uint16_t start_col =
              block_c *
              block_size;  // Left col of the current block in the final image

          if (block_index >= m_blocks.size()) {
            throw std::runtime_error(
                "Error composing image: Not enough blocks provided for image "
                "dimensions.");
          }

          Block& current_block = m_blocks[block_index];
          const std::vector<uint8_t>& block_data =
              current_block.get_decoded_data();
          uint16_t current_block_height = current_block.m_height;
          uint16_t current_block_width = current_block.m_width;

          if (block_data.size() !=
              static_cast<size_t>(current_block_width) * current_block_height) {
            throw std::runtime_error("Error composing image: Block #" +
                                     std::to_string(block_index) +
                                     " has internal data size mismatch.");
          }

          size_t block_data_idx = 0;
          for (uint16_t r = 0; r < current_block_height; ++r) {
            for (uint16_t c = 0; c < current_block_width; ++c) {
              size_t dest_row = start_row + r;
              size_t dest_col = start_col + c;

              if (dest_row >= m_width || dest_col >= m_width) {
                throw std::runtime_error(
                    "Error composing image: Calculated destination index out "
                    "of image bounds.");
              }

              size_t dest_index = dest_row * m_width + dest_col;

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

  void transform() {
    if (m_data.empty()) {
      return;
    }
    // Store the original value of the previous element
    uint8_t prev_original = m_data[0];
    for (uint64_t i = 1; i < m_data.size(); i++) {
      // Store the original value of the current element before modifying it
      uint8_t current_original = m_data[i];
      // Calculate delta based on the *previous* element's original value
      m_data[i] = static_cast<uint8_t>(current_original - prev_original);
      // Update prev_original for the next iteration
      prev_original = current_original;
    }
  }

  void reverse_transform() {
    if (m_data.empty() || !m_model) {
      return;
    }
    // prev holds the reconstructed value of the previous element
    uint8_t prev_reconstructed = m_data[0];  // First element is unchanged
    for (uint64_t i = 1; i < m_data.size(); i++) {
      // Reconstruct the current element: original[i] = delta[i] + original[i-1]
      m_data[i] = static_cast<uint8_t>(m_data[i] + prev_reconstructed);
      // Update prev_reconstructed to the *newly* reconstructed value for the
      // next iteration
      prev_reconstructed = m_data[i];
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
    uint16_t n_blocks_dim = (m_width + block_size - 1) / block_size;
    m_blocks.reserve(static_cast<size_t>(n_blocks_dim) * n_blocks_dim);

    for (uint16_t block_r = 0; block_r < n_blocks_dim; ++block_r) {
      uint16_t start_row = block_r * block_size;

      uint16_t current_block_height =
          std::min<uint16_t>(block_size, m_width - start_row);

      for (uint16_t block_c = 0; block_c < n_blocks_dim; ++block_c) {
        uint16_t start_col = block_c * block_size;
        uint16_t current_block_width =
            std::min<uint16_t>(block_size, m_width - start_col);

        std::vector<uint8_t> block_data;
        block_data.reserve(static_cast<size_t>(current_block_width) *
                           current_block_height);

        for (uint16_t r = start_row; r < start_row + current_block_height;
             ++r) {
          for (uint16_t c = start_col; c < start_col + current_block_width;
               ++c) {
            size_t index = static_cast<size_t>(r) * m_width + c;
            block_data.push_back(m_data[index]);
          }
        }

        if (block_data.size() !=
            static_cast<size_t>(current_block_width) * current_block_height) {
          std::cerr << "Error: Internal logic error. Block data size mismatch."
                    << " Expected: "
                    << (static_cast<size_t>(current_block_width) *
                        current_block_height)
                    << " Got: " << block_data.size() << std::endl;
          throw std::runtime_error("Block data size mismatch.");
        }
        m_blocks.emplace_back(std::move(block_data), current_block_width,
                              current_block_height);
      }
    }
  }

  private:
  std::string m_input_filename;
  std::string m_output_filename;
  std::ofstream o_file_handle;
  uint16_t m_width;
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

  size_t total_size = (TOKEN_CODED_LEN * coded) +
                      (TOKEN_UNCODED_LEN * uncoded) + (4 * 16) + 1 + 1 +
                      (img.m_blocks.size()) * 16;

  size_t size_original = img.get_width() * img.get_width() * 8;
  std::cout << "Original data size: " << size_original << "b" << std::endl;
  std::cout << "Coded tokens: " << coded << " (" << TOKEN_CODED_LEN * coded
            << "b)" << std::endl;
  std::cout << "Uncoded tokens: " << uncoded << " ("
            << TOKEN_UNCODED_LEN * uncoded << "b)" << std::endl;
  std::cout << "Total size (including block headers): " << total_size << "b ("
            << total_size / 8 << "B)" << std::endl;
  std::cout << "Compression ratio: "
            << size_original /
                   static_cast<double>((TOKEN_CODED_LEN * coded) +
                                       (TOKEN_UNCODED_LEN * uncoded))
            << std::endl;
}

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);
  std::cout << "max length: " << MAX_CODED_LEN << std::endl;
  std::cout << "offset bits: " << OFFSET_BITS << std::endl;

  assert(OFFSET_BITS <= 15);
  assert(LENGTH_BITS <= 15);

  if (args.is_compress_mode()) {
    Image i =
        Image(args.get_input_file(), args.get_output_file(),
              args.get_image_width(), args.is_adaptive(), args.use_model());
    if (args.use_model()) {
      i.transform();
    }
    i.create_blocks();
    i.encode_blocks();
    print_final_stats(i);
  } else {
    Image i = Image(args.get_input_file(), args.get_output_file());
    i.decode_blocks();
    i.compose_image();
    i.reverse_transform();
    i.write_dec_output_file();
  }

  return 0;
}