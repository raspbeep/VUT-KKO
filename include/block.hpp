#ifndef LZ_BLOCK_HPP
#define LZ_BLOCK_HPP

#include <array>
#include <cstdint>
#include <vector>

#include "common.hpp"
#include "token.hpp"

class Block {
  public:
  // Constructor for encoding
  Block(const std::vector<uint8_t> data, uint32_t width, uint32_t height);

  // Constructor for decoding
  Block(uint32_t width, uint32_t height, SerializationStrategy strategy);

  void serialize_all_strategies();

  void serialize(SerializationStrategy strategy);

  void deserialize();

  void delta_transform(SerializationStrategy strategy);

  void reverse_delta_transform();

  void mtf(SerializationStrategy strategy);

  void reverse_mtf();

  void insert_token(SerializationStrategy strategy, token_t token);

  void decode_using_strategy(SerializationStrategy strategy);

  void encode_using_strategy(SerializationStrategy strategy);

  void encode_adaptive();

  // debug compare function, can be called after encoding and decoding took
  // place to check if the original data matches the decoded
  void compare_encoded_decoded();

  void print_tokens();

  std::vector<uint8_t>& get_data();

  const std::vector<uint8_t>& get_decoded_data() {
    if (m_picked_strategy == DEFAULT) {
      return m_decoded_data;
    }
    return m_decoded_deserialized_data;
  }

  public:
  std::array<std::vector<uint8_t>, N_STRATEGIES> m_data;
  std::array<std::vector<token_t>, N_STRATEGIES> m_tokens;
  std::array<StrategyResult, N_STRATEGIES> m_strategy_results;
  std::array<uint8_t, N_STRATEGIES> m_delta_params;
  uint32_t m_width;
  uint32_t m_height;
  std::vector<uint8_t> m_decoded_data;
  std::vector<uint8_t> m_decoded_deserialized_data;

  SerializationStrategy m_picked_strategy;
};

#endif  // LZ_CODEC_HPP