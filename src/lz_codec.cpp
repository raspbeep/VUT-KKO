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

class Image {
  public:
  // Constructor for encoding
  Image(std::string i_filename, std::string o_filename, uint16_t width,
        bool adaptive)
      : m_input_filename(i_filename),
        m_output_filename(o_filename),
        m_width(width),
        m_adaptive(adaptive) {
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
    uint16_t offset_bits;
    uint16_t length_bits;
    read_blocks_from_file(m_input_filename, m_width, m_width, offset_bits,
                          length_bits, m_blocks);
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
    // iterate over all blocks, serialize, encode and write them
    for (size_t i = 0; i < m_data.size(); i++) {
      o_file_handle.put(m_data[i]);
    }
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
      block.print_tokens();
    }

    write_blocks_to_stream(m_output_filename, m_width, m_width, OFFSET_BITS,
                           LENGTH_BITS, m_adaptive, m_blocks);
  }

  void decode_blocks() {
    for (size_t i = 0; i < m_blocks.size(); i++) {
      Block& block = m_blocks[i];
      block.decode_using_strategy(DEFAULT);
      m_data.insert(m_data.end(), block.m_decoded_data.begin(),
                    block.m_decoded_data.end());
      block.print_tokens();
    }
  }

  uint16_t get_width() const {
    return m_width;
  }

  private:
  void create_single_block() {
    // single block
    m_blocks.reserve(1);
    m_blocks.push_back(Block(m_data, m_width, m_width));
  }

  void create_multiple_blocks() {
    m_blocks.clear();  // Clear previous blocks if any
    uint16_t n_blocks_dim = (m_width + block_size - 1) / block_size;
    m_blocks.reserve(static_cast<size_t>(n_blocks_dim) * n_blocks_dim);

    // Iterate through block indices (row and column)
    for (uint16_t block_r = 0; block_r < n_blocks_dim; ++block_r) {
      uint16_t start_row = block_r * block_size;
      // Calculate the actual height of the current block (handles boundary)
      uint16_t current_block_height =
          std::min<uint16_t>(block_size, m_width - start_row);

      for (uint16_t block_c = 0; block_c < n_blocks_dim; ++block_c) {
        uint16_t start_col = block_c * block_size;
        uint16_t current_block_width =
            std::min<uint16_t>(block_size, m_width - start_col);

        std::vector<uint8_t> block_data;
        // Reserve exact space needed for the current block's data
        block_data.reserve(static_cast<size_t>(current_block_width) *
                           current_block_height);

        // Iterate through pixels within the current block
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

  size_t total_size =
      (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded) + (8 * 8);

  size_t size_original = img.get_width() * img.get_width() * 8;
  std::cout << "Original data size: " << size_original << "b" << std::endl;
  std::cout << "Coded tokens: " << coded << "(" << TOKEN_CODED_LEN * coded
            << "b)" << std::endl;
  std::cout << "Uncoded tokens: " << uncoded << "("
            << TOKEN_UNCODED_LEN * uncoded << "b)" << std::endl;
  std::cout << "Total size (including block headers): " << total_size << "b\t"
            << total_size / 8 << "B" << std::endl;
  std::cout << "Compression ratio: "
            << size_original /
                   static_cast<double>((TOKEN_CODED_LEN * coded) +
                                       (TOKEN_UNCODED_LEN * uncoded))
            << std::endl;
  std::cout << "Saved: "
            << (size_original - total_size) / static_cast<double>(size_original)
            << "%" << std::endl;
}

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);
  std::cout << "max length: " << MAX_CODED_LEN << std::endl;
  std::cout << "offset bits: " << OFFSET_BITS << std::endl;

  if (args.is_compress_mode()) {
    Image i = Image(args.get_input_file(), args.get_output_file(),
                    args.get_image_width(), args.is_adaptive());
    i.create_blocks();
    i.encode_blocks();
    print_final_stats(i);
  } else {
    Image i = Image(args.get_input_file(), args.get_output_file());
    i.decode_blocks();
    i.write_dec_output_file();
  }

  return 0;
}