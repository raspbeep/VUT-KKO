#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <argparse.hpp>
#include <string>

class ArgumentParser {
  private:
  argparse::ArgumentParser program;
  bool compress_mode;
  bool decompress_mode;
  std::string input_file;
  std::string output_file;
  bool adaptive;
  bool model;
  uint32_t image_width;
  uint16_t offset;
  uint16_t length;

  public:
  ArgumentParser(int argc, char *argv[]);
  bool is_compress_mode() const;
  bool is_decompress_mode() const;
  std::string get_input_file() const;
  std::string get_output_file() const;
  bool is_adaptive() const;
  bool use_model() const;
  uint32_t get_image_width() const;
  void print_args() const;
};

#endif  // ARGUMENT_PARSER_H
