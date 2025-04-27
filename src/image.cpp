/**
 * @file      image.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the Image class implementation
 *
 * @date      12 April  2025 \n
 */

#include "image.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "block.hpp"
#include "block_reader.hpp"
#include "block_writer.hpp"
#include "common.hpp"

// constructor for encoding
Image::Image(std::string i_filename, std::string o_filename, uint32_t width,
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

// constructor for decoding
Image::Image(std::string i_filename, std::string o_filename)
    : m_input_filename(i_filename), m_output_filename(o_filename) {
  // read the input file and store it in m_data vector
  read_dec_input_file();
}

Image::~Image() {
  if (o_file_handle.is_open()) {
    o_file_handle.close();
  }
}

void Image::read_dec_input_file() {
  // read the input file and store the tokens in m_tokens vector
  // store all the params from header in the class variables

  read_blocks_from_file(m_input_filename, m_width, m_height, OFFSET_BITS,
                        LENGTH_BITS, m_adaptive, m_model, m_blocks);
}

void Image::read_enc_input_file() {
  std::ifstream file(m_input_filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Error: Unable to open input file: " +
                             m_input_filename);
  }

  file.seekg(0, std::ios::end);
  auto length = file.tellg();
  file.seekg(0, std::ios::beg);

  uint64_t temp_height = length / m_width;

  if (length > UINT32_MAX) {
    std::ostringstream error_msg;
    error_msg << "Error: Input file '" << m_input_filename
              << "' is too large. Maximum size is " << UINT16_MAX << " bytes.";
    throw std::runtime_error(error_msg.str());
  }
  m_height = static_cast<uint32_t>(temp_height);

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

void Image::write_dec_output_file() {
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

void Image::create_blocks() {
  if (!m_adaptive) {
    create_single_block();
  } else {
    create_multiple_blocks();
  }
}

void Image::print_blocks() {
  for (size_t i = 0; i < m_blocks.size(); i++) {
    Block& block = m_blocks[i];
    std::cout << "Block #" << i << " of width: " << block.m_width
              << " and height: " << block.m_height << std::endl;
  }
}

void Image::encode_blocks() {
  std::vector<token_t> tokens;
  // iterate over all blocks, serialize, encode and write them
  for (size_t i = 0; i < m_blocks.size(); i++) {
    Block& block = m_blocks[i];
    if (m_adaptive) {
      block.serialize_all_strategies();
      if (m_model)
        for (size_t j = 0; j < N_STRATEGIES; j++) {
#if MTF
          block.mtf(static_cast<SerializationStrategy>(j));
#else
          block.delta_transform(static_cast<SerializationStrategy>(j));
#endif
        }
      block.encode_adaptive();
    } else {
      if (m_model)
#if MTF
        block.mtf(DEFAULT);
#else
        block.delta_transform(DEFAULT);
#endif
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

  // m_data will not be needed anymore
  m_data.clear();
}

void Image::write_blocks() {
  write_blocks_to_stream(m_output_filename, m_width, m_height, OFFSET_BITS,
                         LENGTH_BITS, m_adaptive, m_model, m_blocks);
}

void Image::decode_blocks() {
  for (size_t i = 0; i < m_blocks.size(); i++) {
    Block& block = m_blocks[i];
    block.decode_using_strategy(DEFAULT);
#if DEBUG_PRINT_TOKENS
    block.print_tokens();
#endif
    if (m_model) {
#if MTF
      block.reverse_mtf();
#else
      block.reverse_delta_transform();
#endif
    }
    if (m_adaptive) {
      block.deserialize();
    }
  }
}

void Image::compose_image() {
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

    if (single_block.m_width != m_width || single_block.m_height != m_height) {
      std::cout << "Error composing image: Single block dimensions: "
                << single_block.m_width << "x" << single_block.m_height
                << ", expected: " << m_width << "x" << m_height << std::endl;
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

    for (uint16_t block_r = 0; block_r < n_blocks_rows; ++block_r) {
      uint16_t start_row = block_r * BLOCK_SIZE;

      for (uint16_t block_c = 0; block_c < n_blocks_cols; ++block_c) {
        uint16_t start_col = block_c * BLOCK_SIZE;

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

        size_t block_data_idx = 0;
        // place data into the final image using correct destination indices
        for (uint16_t r = 0; r < current_block_height; ++r) {
          for (uint16_t c = 0; c < current_block_width; ++c) {
            size_t dest_row = start_row + r;
            size_t dest_col = start_col + c;

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

uint32_t Image::get_width() {
  return m_width;
}

uint32_t Image::get_height() {
  return m_height;
}

bool Image::is_adaptive() {
  return m_adaptive;
}

bool Image::is_compression_successful() {
  size_t coded = 0;
  size_t uncoded = 0;
  for (auto& block : m_blocks) {
    auto strategy = block.m_picked_strategy;
    coded += block.m_strategy_results[strategy].n_coded_tokens;
    uncoded += block.m_strategy_results[strategy].n_unencoded_tokens;
  }
  size_t file_header_bits =
      32 + 32 + 16 + 16 + 1 +
      1;  // width, height, offset, length, model, adaptive

  // block size
  if (m_adaptive) {
    file_header_bits += 16;
  }

  size_t total_token_bits =
      (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded);

  size_t total_strategy_bits = m_blocks.size() * 2;
  size_t total_size_bits =
      file_header_bits + total_token_bits + total_strategy_bits;

  size_t size_original = static_cast<size_t>(m_width) * m_height;

  size_t compressed_size = static_cast<size_t>(ceil(total_size_bits / 8.0));

  // std::cout << "--- Compression Stats ---" << std::endl;
  std::cout << "Original Size: " << size_original << " bytes" << std::endl;
  std::cout << "Compressed Size: " << compressed_size << " bytes" << std::endl;
  std::cout << "Space saving: "
            << 1 - static_cast<double>(compressed_size) / size_original
            << std::endl;
  return compressed_size < size_original;
}

void Image::reverse_transform() {
  for (auto& block : m_blocks) {
    block.reverse_delta_transform();
  }
}

void Image::copy_unsuccessful_compression() {
  // open the output file in binary mode
  std::ofstream o_file_handle(m_output_filename, std::ios::binary);

  // write a zero byte to the output file
  o_file_handle.put(0);
  // open the input file and copy the data to the output file
  std::ifstream i_file_handle(m_input_filename, std::ios::binary);
  if (!i_file_handle) {
    throw std::runtime_error("Error: Unable to open input file: " +
                             m_input_filename);
  }
  o_file_handle << i_file_handle.rdbuf();
  o_file_handle.close();
  i_file_handle.close();
  std::cout << "Unsuccessful compression, original data copied to: "
            << m_output_filename << std::endl;
}

void Image::create_single_block() {
  // single block
  m_blocks.reserve(1);
  m_blocks.push_back(Block(m_data, m_width, m_width));
}

void Image::create_multiple_blocks() {
  m_blocks.clear();
  uint16_t n_blocks_rows = (m_height + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint16_t n_blocks_cols = (m_width + BLOCK_SIZE - 1) / BLOCK_SIZE;
  m_blocks.reserve(static_cast<size_t>(n_blocks_rows) * n_blocks_cols);

  for (uint16_t block_r = 0; block_r < n_blocks_rows; ++block_r) {
    uint16_t start_row = block_r * BLOCK_SIZE;
    uint16_t current_block_height =
        std::min<uint16_t>(BLOCK_SIZE, m_height - start_row);

    for (uint16_t block_c = 0; block_c < n_blocks_cols; ++block_c) {
      uint16_t start_col = block_c * BLOCK_SIZE;
      uint16_t current_block_width =
          std::min<uint16_t>(BLOCK_SIZE, m_width - start_col);

      std::vector<uint8_t> block_data;
      block_data.reserve(static_cast<size_t>(current_block_width) *
                         current_block_height);

      for (uint16_t r = start_row; r < start_row + current_block_height; ++r) {
        for (uint16_t c = start_col; c < start_col + current_block_width; ++c) {
          size_t index = static_cast<size_t>(r) * m_width + c;
          if (index >= m_data.size()) {
            throw std::runtime_error(
                "Error: Calculated index out of bounds during block "
                "creation.");
          }
          block_data.push_back(m_data[index]);
        }
      }

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

      m_blocks.emplace_back(std::move(block_data), current_block_width,
                            current_block_height);
    }
  }

  if (m_blocks.size() != static_cast<size_t>(n_blocks_rows) * n_blocks_cols) {
    std::cerr << "Warning: Number of created blocks (" << m_blocks.size()
              << ") does not match expected count ("
              << (static_cast<size_t>(n_blocks_rows) * n_blocks_cols) << ")."
              << std::endl;
  }
}
