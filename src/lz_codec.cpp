#include <math.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "argparser.hpp"
#include "bst.hpp"
#include "hashtable.hpp"
#include "token.hpp"

#define DEBUG_DUMMY_SEQ 0
#define DEBUG_PRINT 1
#define DEBUG_COMP_ENC_UNENC 1

#define SEARCH_BUF_SIZE 127

const uint16_t OFFSET_BITS = ceil(log2(SEARCH_BUF_SIZE));  // 7
const uint16_t LENGTH_BITS = ceil(log2(MAX_CODED_LEN));    // 5

const size_t TOKEN_CODED_LEN = 1 + OFFSET_BITS + LENGTH_BITS;  // 13
const size_t TOKEN_UNCODED_LEN = 1 + 8;                        // 8

struct StrategyResult {
  size_t n_coded_tokens;
  size_t n_unencoded_tokens;
};

const uint16_t block_size = 16;

enum SerializationStrategy { HORIZONTAL, VERTICAL, N_STRATEGIES, DEFAULT };

class Block {
  public:
  Block(const std::vector<uint8_t> data, uint16_t width, uint16_t height)
      : m_width(width), m_height(height), m_picked_strategy(HORIZONTAL) {
    for (size_t i = 0; i < SerializationStrategy::N_STRATEGIES; i++) {
      m_data[i].reserve(width * height);
    }
    m_data[0].assign(data.begin(), data.end());
    m_strategy_results.fill({0, 0});
  }

  void serialize_all_strategies() {
    serialize(HORIZONTAL);
    serialize(VERTICAL);
    // serialize(DIAGONAL);
  }

  void serialize(enum SerializationStrategy strategy) {
    switch (strategy) {
      case HORIZONTAL:
        // default, does not need to be serialized
        return;
      case VERTICAL:
        m_data[VERTICAL].clear();
        m_data[VERTICAL].reserve(m_width * m_height);
        for (size_t j = 0; j < m_width; ++j) {
          for (size_t i = 0; i < m_height; ++i) {
            m_data[VERTICAL].push_back(m_data[HORIZONTAL][i * m_width + j]);
          }
        }
        break;
      default:
        break;
    }
  }

  void delta_transform(bool adaptive) {
    if (adaptive) {
      // find minimum in each m_data vector
      for (size_t i = 0; i < SerializationStrategy::N_STRATEGIES; i++) {
        uint8_t min = *std::min_element(m_data[i].begin(), m_data[i].end());
        m_delta_params[i] = min;
        for (size_t j = 0; j < m_data[i].size(); j++) {
          m_data[i][j] -= min;
        }
      }
    } else {
      uint8_t min = *std::min_element(m_data[0].begin(), m_data[0].end());
      m_delta_params[0] = min;
      for (size_t j = 0; j < m_data[0].size(); j++) {
        m_data[0][j] -= min;
      }
    }
  }

  void insert_token(SerializationStrategy strategy, token_t token) {
#if 0
    std::cout << "insert_token: " << std::endl;
    std::cout << "  strategy: " << strategy << std::endl;
    std::cout << "  token: ";
    if (token.coded) {
      std::cout << "coded: " << static_cast<int>(token.data.offset) << " "
                << static_cast<int>(token.data.length) << std::endl;
    } else {
      std::cout << "uncoded: " << static_cast<int>(token.data.value) << "("
                << static_cast<char>(token.data.value) << ")" << std::endl;
    }
#endif
    m_tokens[strategy].push_back(token);
    // add to the strategy result for picking the best one in adaptive
    token.coded ? m_strategy_results[strategy].n_coded_tokens++
                : m_strategy_results[strategy].n_unencoded_tokens++;
  }

  void decode_using_strategy(SerializationStrategy strategy) {
    if (strategy == SerializationStrategy::DEFAULT) {
      strategy = m_picked_strategy;
    }
    auto tokens = m_tokens[strategy];
    uint64_t position = 0;
    for (size_t i = 0; i < tokens.size(); i++) {
      token_t token = tokens[i];
      if (token.coded) {
        // coded token
        uint64_t token_position = position - token.data.offset;
        uint16_t length = token.data.length + MIN_CODED_LEN;
        for (uint16_t j = 0; j < length; j++) {
          m_decoded_data.push_back(m_decoded_data[token_position + j]);
        }
#if 0
        std::cout << "decoded: " << static_cast<int>(token.data.offset) << " "
                  << static_cast<int>(token.data.length) << std::endl;
        std::cout << "decoded: ";
        for (uint16_t j = token_position; j < token_position + length; j++) {
          std::cout << static_cast<int>(m_decoded_data[j]) << " ";
        }
        std::cout << "(";
        for (uint16_t j = 0; j < length; j++) {
          std::cout << static_cast<char>(m_decoded_data[j]);
        }
        std::cout << ")" << std::endl;
#endif
        position += length;

      } else {
        // uncoded token
        m_decoded_data.push_back(token.data.value);
#if 0
        std::cout << "decoded: " << static_cast<int>(token.data.value) << "("
                  << static_cast<char>(token.data.value) << ")" << std::endl;
#endif
        position++;
      }
    }
#if 0
    std::cout << "decoded data: ";
    for (size_t i = 0; i < m_decoded_data.size(); i++) {
      std::cout << static_cast<int>(m_decoded_data[i]) << " ";
    }
    std::cout << std::endl;
    std::cout << "decoded data: \t";
    for (size_t i = 0; i < m_decoded_data.size(); i++) {
      std::cout << static_cast<char>(m_decoded_data[i]);
    }
    std::cout << std::endl;
#endif
  }

  void encode_using_strategy(SerializationStrategy strategy) {
    // if the strategy is not set, use the horizontal one
    if (strategy == SerializationStrategy::DEFAULT) {
      strategy = SerializationStrategy::HORIZONTAL;
    }

    auto hash_table = HashTable(1 << 12);
    // push the first two bytes unencoded since the dict is empty
    uint64_t position = 0;
    for (position = 0; position < MIN_CODED_LEN; position++) {
      insert_token(strategy, {.coded = false,
                              .data = {.value = m_data[strategy][position]}});
    }

    hash_table.insert(m_data[strategy], 0);
    uint64_t next_pos;
    uint64_t removed_until = 0;
    // iterate over all bytes of the input
    for (position = MIN_CODED_LEN, next_pos = MIN_CODED_LEN;
         position < m_data[strategy].size();) {
      // search for the longest prefix in the hash table
      search_result result = hash_table.search(m_data[strategy], position);
      next_pos = position + result.length;
      if (result.found) {
        next_pos += MIN_CODED_LEN;
        // found a match, push the token
        insert_token(strategy, {.coded = true,
                                .data = {.offset = static_cast<uint16_t>(
                                             position - result.position),
                                         .length = result.length}});
      } else {
        // no match found, push the byte unencoded
        insert_token(strategy, {.coded = false,
                                .data = {.value = m_data[strategy][position]}});
        next_pos++;
      }

      // insert new prefixes into the hash table
      while (position < next_pos) {
        hash_table.insert(m_data[strategy], position - MIN_CODED_LEN + 1);
        position++;
      }

      // remove old entries from the hash table
      if (position > SEARCH_BUF_SIZE) {
        size_t remove_from = removed_until;
        size_t remove_to = position - SEARCH_BUF_SIZE - 1;
        for (size_t r = remove_from; r <= remove_to; r++) {
          hash_table.remove(m_data[strategy], r);
        }
        removed_until = remove_to + 1;
      }

      // std::cout << "position: " << position << std::endl;
    }
  }

  void encode_adaptive() {
    size_t best_encoded_size, current_strategy_result;
    bool first = true;
    for (size_t i = 0; i < SerializationStrategy::N_STRATEGIES; i++) {
      // delta_transform(true);
      encode_using_strategy(static_cast<SerializationStrategy>(i));
      current_strategy_result =
          m_strategy_results[i].n_coded_tokens * TOKEN_CODED_LEN +
          m_strategy_results[i].n_unencoded_tokens * TOKEN_UNCODED_LEN;
      if (first || current_strategy_result < best_encoded_size) {
        best_encoded_size = current_strategy_result;
        m_picked_strategy = static_cast<SerializationStrategy>(i);
        first = false;
      }
    }
  }

  // debug compare function, can be called after encoding and decoding took
  // place to check if the original data matches the decoded
  void compare_encoded_decoded() {
    bool identical = true;
    for (size_t i = 0; i < m_data[m_picked_strategy].size(); i++) {
      if (m_decoded_data[i] != m_data[m_picked_strategy][i]) {
        std::cout
            << "Error: Decoded data does not match original data at index " << i
            << ": " << static_cast<int>(m_decoded_data[i])
            << " != " << static_cast<int>(m_data[m_picked_strategy][i])
            << std::endl;
        identical = false;
      }
    }
    if (identical) {
      std::cout << "Decoded data matches original data." << std::endl;
    } else {
      std::cout << "Decoded data does not match original data." << std::endl;
      throw std::runtime_error(
          "Error: Decoded data does not match original data.");
    }
  }

  std::vector<uint8_t>& get_data() {
    return m_data[m_picked_strategy];
  }

  public:
  std::array<std::vector<uint8_t>, SerializationStrategy::N_STRATEGIES> m_data;
  std::array<std::vector<token_t>, SerializationStrategy::N_STRATEGIES>
      m_tokens;
  std::array<StrategyResult, SerializationStrategy::N_STRATEGIES>
      m_strategy_results;
  std::array<uint8_t, SerializationStrategy::N_STRATEGIES> m_delta_params;
  uint16_t m_width;
  uint16_t m_height;
  std::vector<uint8_t> m_decoded_data;

  SerializationStrategy m_picked_strategy;
};

class Image {
  public:
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

  ~Image() {
    if (o_file_handle.is_open()) {
      o_file_handle.close();
    }
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

  void begin_enc_output_file() {
    o_file_handle.open(m_output_filename, std::ios::binary | std::ios::trunc);
    if (!o_file_handle) {
      throw std::runtime_error("Error: Unable to open output file: " +
                               m_output_filename);
    }
    // write header (2B for width and 2B for height)
    o_file_handle.write(reinterpret_cast<const char*>(&m_width),
                        sizeof(m_width));
    o_file_handle.write(reinterpret_cast<const char*>(&m_width),
                        sizeof(m_width));
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

  void append_encoded_block_to_file(Block& block) {
    // write the block to the output file
    o_file_handle.write(reinterpret_cast<const char*>(&block.m_width),
                        sizeof(block.m_width));
    o_file_handle.write(reinterpret_cast<const char*>(&block.m_height),
                        sizeof(block.m_height));
    write_tokens_to_file(o_file_handle, block.m_tokens[block.m_picked_strategy],
                         OFFSET_BITS, LENGTH_BITS);
  }

  void encode_blocks() {
    begin_enc_output_file();

    // iterate over all blocks, serialize, encode and write them
    for (size_t i = 0; i < m_blocks.size(); i++) {
      Block& block = m_blocks[i];
      // block.delta_transform(m_adaptive);
      if (m_adaptive) {
        block.serialize_all_strategies();
        block.encode_adaptive();
      } else {
        block.encode_using_strategy(SerializationStrategy::DEFAULT);
      }
#if DEBUG_PRINT
      std::cout << "Block #" << i
                << " picked strategy: " << block.m_picked_strategy << std::endl;
#endif
#if DEBUG_COMP_ENC_UNENC
      block.decode_using_strategy(SerializationStrategy::DEFAULT);
      block.compare_encoded_decoded();
#endif
      // appends the best strategy encoded block to the output file
      append_encoded_block_to_file(block);
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

  public:
  std::vector<Block> m_blocks;
};

void print_final_stats(Image& img) {
  size_t coded = 0;
  size_t uncoded = 0;
  size_t n_blocks = img.m_blocks.size();
  for (auto& block : img.m_blocks) {
    auto strategy = block.m_picked_strategy;
    for (auto& token : block.m_tokens[strategy]) {
      token.coded ? coded++ : uncoded++;
    }
  }

  size_t size_original = img.get_width() * img.get_width() * 8;
  std::cout << "Original data size: " << size_original << "b" << std::endl;
  std::cout << "Coded tokens: " << coded << "(" << TOKEN_CODED_LEN * coded
            << "b)" << std::endl;
  std::cout << "Uncoded tokens: " << uncoded << "("
            << TOKEN_UNCODED_LEN * uncoded << "b)" << std::endl;
  std::cout << "Total size (including block headers): "
            << (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded) +
                   (n_blocks * 32)
            << "b\t"
            << ((TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded) +
                (n_blocks * 32)) /
                   8
            << "B" << std::endl;
  std::cout << "Compression ratio: "
            << static_cast<double>((TOKEN_CODED_LEN * coded) +
                                   (TOKEN_UNCODED_LEN * uncoded)) /
                   size_original
            << std::endl;
}

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);
  std::vector<uint8_t> data;

#if DEBUG_DUMMY_SEQ
  uint8_t sequence[] = {97, 97, 99, 97, 97, 99, 97, 97, 99, 97,  97, 99,
                        97, 97, 99, 97, 97, 99, 97, 97, 97, 99,  97, 97,
                        97, 98, 99, 97, 98, 97, 97, 97, 99, 100, 97, 100};

  std::vector<uint8_t> sequence_vec(std::begin(sequence), std::end(sequence));
  Image i = Image(sequence_vec, 6, args.is_adaptive());
#else
  Image i = Image(args.get_input_file(), args.get_output_file(),
                  args.get_image_width(), args.is_adaptive());
#endif

  i.create_blocks();
  i.encode_blocks();

  print_final_stats(i);

  return 0;
}