/**
 * @file      block.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for block class implementation for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#ifndef LZ_BLOCK_HPP
#define LZ_BLOCK_HPP

#include <array>
#include <cstdint>
#include <vector>

#include "common.hpp"
#include "token.hpp"

/**
 * @class Block
 * @brief Represents a block of data for LZSS compression/decompression,
 * potentially handling different serialization strategies.
 */
class Block {
  public:
  /**
   * @brief Constructor for encoding. Initializes a block with data.
   * @param data The raw data for the block.
   * @param width The width of the block (relevant for image data).
   * @param height The height of the block (relevant for image data).
   */
  Block(const std::vector<uint8_t> data, uint32_t width, uint32_t height);

  /**
   * @brief Constructor for decoding. Initializes an empty block with dimensions
   * and the strategy used for encoding.
   * @param width The width of the block.
   * @param height The height of the block.
   * @param strategy The serialization strategy used during encoding.
   */
  Block(uint32_t width, uint32_t height, SerializationStrategy strategy);

  /**
   * @brief Applies serialization transformations for all supported strategies.
   */
  void serialize_all_strategies();

  /**
   * @brief Applies a specific serialization transformation (e.g., vertical
   * scan) to the block's data.
   * @param strategy The serialization strategy to apply.
   */
  void serialize(SerializationStrategy strategy);

  /**
   * @brief Reverses the serialization transformation applied during encoding.
   */
  void deserialize();

  /**
   * @brief Applies delta transformation (difference coding) to the data for a
   * specific strategy.
   * @param strategy The serialization strategy whose data to transform.
   */
  void delta_transform(SerializationStrategy strategy);

  /**
   * @brief Reverses the delta transformation applied during encoding.
   */
  void reverse_delta_transform();

  /**
   * @brief Applies Move-To-Front (MTF) transformation to the data for a
   * specific strategy.
   * @param strategy The serialization strategy whose data to transform.
   */
  void mtf(SerializationStrategy strategy);

  /**
   * @brief Reverses the Move-To-Front (MTF) transformation applied during
   * encoding.
   */
  void reverse_mtf();

  /**
   * @brief Inserts an LZSS token into the token list for a specific strategy.
   * @param strategy The serialization strategy the token belongs to.
   * @param token The token to insert.
   */
  void insert_token(SerializationStrategy strategy, token_t token);

  /**
   * @brief Decodes the block using the tokens associated with a specific
   * strategy.
   * @param strategy The strategy whose tokens to use for decoding. If DEFAULT,
   * uses the picked strategy.
   */
  void decode_using_strategy(SerializationStrategy strategy);

  /**
   * @brief Encodes the block using LZSS for a specific strategy, generating
   * tokens.
   * @param strategy The strategy to use for encoding. If DEFAULT, uses
   * HORIZONTAL.
   */
  void encode_using_strategy(SerializationStrategy strategy);

  /**
   * @brief Encodes the block using all strategies and picks the one resulting
   * in the smallest encoded size.
   */
  void encode_adaptive();

  /**
   * @brief Compares the original data (for the picked strategy) with the
   * decoded data. Throws if they don't match. (Debug function)
   */
  void compare_encoded_decoded();

  /**
   * @brief Prints the generated tokens for the picked strategy to standard
   * output. (Debug function)
   */
  void print_tokens();

  /**
   * @brief Gets a reference to the internal data vector for the picked
   * strategy.
   * @return A reference to the data vector.
   */
  std::vector<uint8_t>& get_data();

  /**
   * @brief Gets a constant reference to the decoded data vector.
   * @return A constant reference to the decoded data (potentially after
   * deserialization).
   */
  const std::vector<uint8_t>& get_decoded_data() {
    // If deserialization happened (strategy wasn't HORIZONTAL), return the
    // deserialized data. Otherwise, return the directly decoded data.
    if (m_picked_strategy != HORIZONTAL &&
        !m_decoded_deserialized_data.empty()) {
      return m_decoded_deserialized_data;
    }
    return m_decoded_data;
  }

  public:
  // Internal data storage for different serialization strategies
  std::array<std::vector<uint8_t>, N_STRATEGIES> m_data;
  // Storage for generated tokens for different strategies
  std::array<std::vector<token_t>, N_STRATEGIES> m_tokens;
  // Stores results (token counts) for each strategy
  std::array<StrategyResult, N_STRATEGIES> m_strategy_results;
  // Parameters for delta transformation (if used)
  std::array<uint8_t, N_STRATEGIES> m_delta_params;
  // Block dimensions
  uint32_t m_width;
  uint32_t m_height;
  // Data buffer used during decoding before potential deserialization
  std::vector<uint8_t> m_decoded_data;
  // Data buffer used after deserialization during decoding
  std::vector<uint8_t> m_decoded_deserialized_data;
  // The serialization strategy chosen (either fixed or adaptively determined)
  SerializationStrategy m_picked_strategy;
};

#endif  // LZ_BLOCK_HPP
