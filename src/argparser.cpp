/**
 * @file      argparser.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Argument parser implementation
 *
 * @date      12 April  2025 \n
 */

#include "argparser.hpp"

#include <argparse.hpp>
#include <iostream>

#include "common.hpp"

ArgumentParser::ArgumentParser(int argc, char *argv[])
    : program("lz_codec", "1.0", argparse::default_arguments::help) {
  auto &group = program.add_mutually_exclusive_group();
  group.add_argument("-c")
      .default_value(false)
      .implicit_value(true)
      .store_into(compress_mode)
      .help("Compress mode");
  group.add_argument("-d")
      .default_value(false)
      .implicit_value(true)
      .store_into(decompress_mode)
      .help("Decompress mode");
  program.add_argument("-i")
      .required()
      .store_into(input_file)
      .help("Input file")
      .metavar("INPUT");
  program.add_argument("-o")
      .required()
      .store_into(output_file)
      .help("Output file")
      .metavar("OUTPUT");
  program.add_argument("-a")
      .default_value(false)
      .implicit_value(true)
      .store_into(adaptive)
      .help("Use adaptive strategy");
  program.add_argument("-m")
      .default_value(false)
      .implicit_value(true)
      .store_into(model)
      .help("Use model preprocessing");
  program.add_argument("-w")
      .default_value<uint32_t>(1)
      .scan<'i', uint32_t>()
      .store_into(image_width)
      .help("Image width")
      .nargs(1)
      .metavar("WIDTH");
  program.add_argument("--block_size")
      .default_value<uint16_t>(DEFAULT_BLOCK_SIZE)
      .scan<'i', uint16_t>()
      .store_into(BLOCK_SIZE)
      .help("Block size (for adaptive mode)")
      .nargs(1)
      .metavar("BLOCK_SIZE");
  program.add_argument("--offset_bits")
      .default_value<uint16_t>(DEFAULT_OFFSET_BITS)
      .scan<'i', uint16_t>()
      .store_into(OFFSET_BITS)
      .help("Number of bits used for offset in token")
      .nargs(1)
      .metavar("OFFSET_BITS");
  program.add_argument("--length_bits")
      .default_value<uint16_t>(DEFAULT_LENGTH_BITS)
      .scan<'i', uint16_t>()
      .store_into(LENGTH_BITS)
      .help("Number of bits used for length in token")
      .nargs(1)
      .metavar("LENGTH_BITS");

  try {
    program.parse_args(argc, argv);
    if (!compress_mode && !decompress_mode) {
      throw std::runtime_error(
          "Error: Missing required argument '-c' or '-d' choosing compression "
          "or decompression respectively.");
    }
    if (decompress_mode && program.is_used("-w")) {
      std::cout << "Warning: Decompress mode is enabled, but width is "
                   "specified. Width "
                   "will be ignored."
                << std::endl;
    }
    if (program.is_used("--block_size")) {
      if (compress_mode && !program.is_used("-a")) {
        std::cout << "Block size was specified but adaptive mode is disabled. "
                     "Ignoring."
                  << std::endl;
      } else if (compress_mode && program.is_used("-a")) {
        std::cout << "Using block size of " << BLOCK_SIZE << std::endl;
      } else {
        std::cout << "Block size was specified but compression mode is "
                     "disabled. Ignoring."
                  << std::endl;
      }
    }
    if (program.is_used("--offset_bits")) {
      std::cout << "Using " << OFFSET_BITS << "b for offset in token"
                << std::endl;
    }
    if (program.is_used("--length_bits")) {
      std::cout << "Using " << LENGTH_BITS << "b for length in token"
                << std::endl;
    }
    if (program.is_used("-a")) {
      std::cout << "adaptive" << std::endl;
      BLOCK_SIZE = 64;
    }
    // print_args();
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    exit(1);
  }
}

bool ArgumentParser::is_compress_mode() const {
  return compress_mode;
}
bool ArgumentParser::is_decompress_mode() const {
  return decompress_mode;
}
std::string ArgumentParser::get_input_file() const {
  return input_file;
}
std::string ArgumentParser::get_output_file() const {
  return output_file;
}
bool ArgumentParser::is_adaptive() const {
  return adaptive;
}
bool ArgumentParser::use_model() const {
  return model;
}
uint32_t ArgumentParser::get_image_width() const {
  return image_width;
}

void ArgumentParser::print_args() const {
  compress_mode ? std::cout << "Compress mode" << std::endl
                : std::cout << "Decompress mode" << std::endl;
  std::cout << "Input file: " << input_file << std::endl;
  std::cout << "Output file: " << output_file << std::endl;
  std::cout << "Adaptive strategy: " << adaptive << std::endl;
  std::cout << "Model preprocessing: " << model << std::endl;
  std::cout << "Image width: " << image_width << std::endl;
}
