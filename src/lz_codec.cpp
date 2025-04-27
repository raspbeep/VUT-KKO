/**
 * @file      lz_codec.c
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     LZSS Compressor and Decompressor main file
 *
 * @date      12 April  2025 \n
 */

#include <assert.h>
#include <math.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "argparser.hpp"
#include "image.hpp"

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
      32 + 32 + 16 + 16 + 1 +
      1;  // width, height, offset, length, model, adaptive

  // block size
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

bool copy_uncompressed_file(std::string input_filename,
                            std::string output_filename) {
  // open the input file in binary mode
  std::ifstream i_file_handle(input_filename, std::ios::binary);

  // read the first byte from the input file
  char first_byte;
  i_file_handle.get(first_byte);

  if (first_byte) {
    i_file_handle.close();

    return false;
  }

  // open the output file
  std::ifstream o_file_handle(output_filename, std::ios::binary);
  // copy the contents of input file into the output file
  std::ofstream o_file_handle_copy(output_filename, std::ios::binary);
  if (!o_file_handle_copy) {
    throw std::runtime_error("Error: Unable to open output file: " +
                             output_filename);
  }
  o_file_handle_copy << i_file_handle.rdbuf();
  o_file_handle_copy.close();
  i_file_handle.close();
  return true;
}

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);

  SEARCH_BUF_SIZE = (1 << OFFSET_BITS) - 1;
  MAX_CODED_LEN = (1 << LENGTH_BITS) - 1 + MIN_CODED_LEN;
  TOKEN_CODED_LEN = 1 + OFFSET_BITS + LENGTH_BITS;
  TOKEN_UNCODED_LEN = 1 + 8;

  // asserts for checking valid values
  assert(BLOCK_SIZE > 0 && BLOCK_SIZE < (1 << 15));
  assert(OFFSET_BITS > 0 && OFFSET_BITS < 16);
  assert(LENGTH_BITS > 0 && LENGTH_BITS < 16);
  assert(MIN_CODED_LEN > 0);

  if (args.is_compress_mode()) {
    Image i =
        Image(args.get_input_file(), args.get_output_file(),
              args.get_image_width(), args.is_adaptive(), args.use_model());
    i.create_blocks();
    i.encode_blocks();
    if (i.is_compression_successful()) {
      i.write_blocks();
    } else {
      i.copy_unsuccessful_compression();
      // print_final_stats(i);
    }
  } else {
    if (copy_uncompressed_file(args.get_input_file(), args.get_output_file())) {
      return 0;
    }
    Image i = Image(args.get_input_file(), args.get_output_file());
    i.decode_blocks();
    i.compose_image();
    i.write_dec_output_file();
  }

  return 0;
}