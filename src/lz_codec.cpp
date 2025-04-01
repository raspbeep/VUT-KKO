#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "argparser.hpp"

std::vector<uint8_t> read_input_file(std::string filename, uint16_t width) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Error: Unable to open input file.");
  }

  file.seekg(0, std::ios_base::end);
  auto length = file.tellg();
  file.seekg(0, std::ios_base::beg);
  if (length != width * width) {
    // change this so the error displays the argument and the actual size
    // of the file
    throw std::runtime_error(
        std::string("Error: Input file size does not match the specified "
                    "width. Expected ") +
        std::to_string(width * width) + " bytes, got " +
        std::to_string(length) + " bytes.");
  }
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
}

class Compressor {
  private:
};

int main(int argc, char *argv[]) {
  ArgumentParser args(argc, argv);
  auto d = read_input_file(args.get_input_file(), args.get_image_width());

  return 0;
}