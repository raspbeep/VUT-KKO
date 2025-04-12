/**
 * @file      argparser.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the argument parser implementation
 *
 * @date      12 April  2025 \n
 */

#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <argparse.hpp>
#include <string>

/**
 * @class ArgumentParser
 * @brief Parses and stores command-line arguments for the lz_codec program.
 */
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
  /**
   * @brief Constructs the ArgumentParser and parses command-line arguments.
   * @param argc Argument count.
   * @param argv Argument vector.
   */
  ArgumentParser(int argc, char *argv[]);

  /**
   * @brief Checks if compress mode is enabled.
   * @return True if compress mode is set, false otherwise.
   */
  bool is_compress_mode() const;

  /**
   * @brief Checks if decompress mode is enabled.
   * @return True if decompress mode is set, false otherwise.
   */
  bool is_decompress_mode() const;

  /**
   * @brief Gets the input file path.
   * @return The input file path as a string.
   */
  std::string get_input_file() const;

  /**
   * @brief Gets the output file path.
   * @return The output file path as a string.
   */
  std::string get_output_file() const;

  /**
   * @brief Checks if the adaptive strategy is enabled.
   * @return True if adaptive strategy is enabled, false otherwise.
   */
  bool is_adaptive() const;

  /**
   * @brief Checks if model preprocessing is enabled.
   * @return True if model preprocessing is enabled, false otherwise.
   */
  bool use_model() const;

  /**
   * @brief Gets the specified image width (used in model preprocessing).
   * @return The image width as a 32-bit unsigned integer.
   */
  uint32_t get_image_width() const;

  /**
   * @brief Prints the parsed arguments to standard output.
   */
  void print_args() const;
};

#endif  // ARGUMENT_PARSER_H
