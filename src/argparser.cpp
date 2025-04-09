#include "argparser.hpp"

#include <argparse.hpp>
#include <iostream>

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
      .scan<'i', uint16_t>()
      .store_into(image_width)
      .help("Output file")
      .metavar("WIDTH");

  // program.add_argument("--offset")
  //     .scan<'i', uint16_t>()
  //     .store_into(offset)
  //     .help("Output file")
  //     .metavar("WIDTH");

  try {
    program.parse_args(argc, argv);
    if (compress_mode && !program.is_used("-w")) {
      throw std::runtime_error(
          "Error: Missing required argument '-w' for decompression mode.");
    }
    if (decompress_mode && program.is_used("-w")) {
      std::cout << "Warning: Decompress mode is enabled, but width is "
                   "specified. Width "
                   "will be ignored."
                << std::endl;
    }
    print_args();
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
uint16_t ArgumentParser::get_image_width() const {
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
