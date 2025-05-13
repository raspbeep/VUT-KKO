/**
 * @file      image.hpp
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

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <fstream>  // Include for std::ofstream
#include <string>
#include <vector>

#include "block.hpp"
#include "common.hpp"
#include "token.hpp"  // Include for token_t

/**
 * @class Image
 * @brief Represents an image or raw data file, handling its division into
 * blocks, encoding/decoding processes, and file I/O.
 */
class Image {
  public:
  /**
   * @brief Constructor for encoding mode. Reads input file based on width.
   * @param i_filename Path to the input file.
   * @param o_filename Path to the output file.
   * @param width Width of the image/data (used to calculate height).
   * @param adaptive Whether to use adaptive block strategy.
   * @param model Whether to use model preprocessing (delta/MTF).
   */
  Image(std::string i_filename, std::string o_filename, uint32_t width,
        bool adaptive, bool model);

  /**
   * @brief Constructor for decoding mode. Reads header and blocks from input
   * file.
   * @param i_filename Path to the compressed input file.
   * @param o_filename Path to the output file for decoded data.
   */
  Image(std::string i_filename, std::string o_filename);

  /**
   * @brief Destructor. Closes the output file handle if open.
   */
  ~Image();

  /**
   * @brief Reads the compressed input file (header and blocks) for decoding.
   */
  void read_dec_input_file();

  /**
   * @brief Reads the raw input file for encoding, calculating height based on
   * width.
   */
  void read_enc_input_file();

  /**
   * @brief Writes the final decoded and composed data to the output file.
   */
  void write_dec_output_file();

  /**
   * @brief Creates blocks from the loaded image data based on adaptive mode
   * setting.
   */
  void create_blocks();

  /**
   * @brief Prints information about the created blocks (dimensions). (Debug
   * function)
   */
  void print_blocks();

  /**
   * @brief Encodes all created blocks, applying model preprocessing and
   * adaptive strategy if enabled.
   */
  void encode_blocks();

  /**
   * @brief Writes the encoded blocks (including header) to the output file.
   */
  void write_blocks();

  /**
   * @brief Decodes all loaded blocks, applying reverse model transformations
   * and deserialization if needed.
   */
  void decode_blocks();

  /**
   * @brief Composes the final image data by assembling the decoded blocks.
   */
  void compose_image();

  /**
   * @brief Gets the width of the image.
   * @return Image width.
   */
  uint32_t get_width();

  /**
   * @brief Gets the height of the image.
   * @return Image height.
   */
  uint32_t get_height();

  /**
   * @brief Checks if adaptive mode is enabled for this image instance.
   * @return True if adaptive mode is enabled, false otherwise.
   */
  bool is_adaptive();

  /**
   * @brief Calculates and checks if the compression resulted in a smaller file
   * size. Prints stats.
   * @return True if compressed size is smaller than original, false otherwise.
   */
  bool is_compression_successful();

  /**
   * @brief Applies the reverse delta transform to all blocks (used during
   * decoding if delta was applied).
   */
  void reverse_transform();  // Consider renaming if MTF is also possible

  /**
   * @brief Copies the original input file to the output file with a leading
   * zero byte, indicating unsuccessful compression.
   */
  void copy_unsuccessful_compression();

  private:
  /**
   * @brief Creates a single block containing the entire image data (used when
   * adaptive mode is off).
   */
  void create_single_block();

  /**
   * @brief Creates multiple blocks by dividing the image data according to
   * BLOCK_SIZE (used when adaptive mode is on).
   */
  void create_multiple_blocks();

  private:
  std::string m_input_filename;
  std::string m_output_filename;
  std::ofstream o_file_handle;  // File handle for potential direct writing
  uint32_t m_width;
  uint32_t m_height;
  bool m_adaptive;
  bool m_model;
  std::vector<uint8_t> m_data;    // Holds raw data for encoding or decoded data
  std::vector<token_t> m_tokens;  // Potentially unused if blocks hold tokens
  bool m_binary_only;

  public:
  std::vector<Block> m_blocks;  // Holds the blocks for processing
};

#endif  // IMAGE_HPP
