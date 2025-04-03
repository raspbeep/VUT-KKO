#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "argparser.hpp"
#include "bst.cpp"

const uint16_t block_size = 16;

enum SerializationStrategy { HORIZONTAL, VERTICAL, DIAGONAL, N_STRATEGIES };

std::vector<uint8_t> read_input_file(const std::string& filename,
                                     uint16_t width) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Error: Unable to open input file: " + filename);
  }

  file.seekg(0, std::ios::end);
  auto length = file.tellg();
  file.seekg(0, std::ios::beg);

  uint64_t expected_size = static_cast<uint64_t>(width) * width;
  if (length < 0 || static_cast<uint64_t>(length) != expected_size) {
    std::ostringstream error_msg;
    error_msg << "Error: Input file '" << filename
              << "' size mismatch. Expected " << expected_size << " bytes ("
              << width << "x" << width << "), but got " << length << " bytes.";
    throw std::runtime_error(error_msg.str());
  }

  std::vector<uint8_t> buffer;
  buffer.reserve(expected_size);
  buffer.assign((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

  if (buffer.size() != expected_size) {
    std::ostringstream error_msg;
    error_msg << "Error: Read incomplete data from file '" << filename
              << "'. Expected " << expected_size << " bytes, read "
              << buffer.size() << " bytes.";
    throw std::runtime_error(error_msg.str());
  }

  return buffer;
}

class Block {
  public:
  Block(const std::vector<uint8_t> data, uint16_t width, uint16_t height)
      : m_width(width), m_height(height) {
    for (size_t i = 0; i < SerializationStrategy::N_STRATEGIES; i++)
      m_data[i].reserve(width * height);
    m_data[0].assign(data.begin(), data.end());
  }

  void serialize_all_strategies() {
    serialize(HORIZONTAL);
    serialize(VERTICAL);
    serialize(DIAGONAL);
  }

  void serialize(enum SerializationStrategy strategy) {
    switch (strategy) {
      case HORIZONTAL:
        // default, does not need to be serialized
        return;
      case VERTICAL:
        for (size_t i = 0; i < m_height; i++) {
          for (size_t j = 0; j < m_width; j++) {
            m_data[VERTICAL].push_back(m_data[HORIZONTAL][i * m_width + j]);
          }
        }
        break;
      case DIAGONAL:
        // TODO: implement diagonal serialization
        return;
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

  // TODO: change to private
  public:
  std::array<std::vector<uint8_t>, SerializationStrategy::N_STRATEGIES> m_data;
  std::array<uint8_t, SerializationStrategy::N_STRATEGIES> m_delta_params;
  uint16_t m_width;
  uint16_t m_height;
};

class Image {
  public:
  Image(const std::vector<uint8_t>& data, uint16_t width, bool adaptive)
      : m_data(data), m_width(width), m_adaptive(adaptive) {
    if (data.size() != static_cast<size_t>(width) * width) {
      throw std::runtime_error(
          "Error: Data size does not match image dimensions.");
    }
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
  std::vector<uint8_t> m_data;
  uint16_t m_width;
  bool m_adaptive;

  public:
  std::vector<Block> m_blocks;
};

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);
  std::vector<uint8_t> data;
  data.reserve(512 * 512);
  for (int i = 0; i < 512 * 512; i++) {
    data.push_back(static_cast<uint8_t>(i % 256));
  }
  Image i =
      Image(read_input_file(args.get_input_file(), args.get_image_width()),
            args.get_image_width(), args.is_adaptive());

  // Image i = Image(data, args.get_image_width(), args.is_adaptive());

  i.create_blocks();
  // i.print_blocks();

  // auto b = i.m_blocks[0];
  // for (size_t i = 0; i < b.m_height; i++) {
  //   for (size_t j = 0; j < b.m_width; j++)
  //     std::cout << static_cast<int>(b.m_data[0][i * b.m_width + j]) << " ";
  // }

  auto tree = BSTree();

  tree.insert_data(16, {1, 2, 3, 4, 5});
  tree.insert_data(15, {6, 7, 8, 9, 10});
  tree.insert_data(14, {11, 12, 13, 14, 15});
  tree.print_recursive_preorder(tree.root);

  return 0;
}