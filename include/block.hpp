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
  Block(const std::vector<uint8_t> data, uint16_t width, uint16_t height);

  // Constructor for decoding
  Block(uint16_t width, uint16_t height, SerializationStrategy strategy);

  void serialize_all_strategies();

  void serialize(SerializationStrategy strategy);

  void delta_transform(bool adaptive);

  void insert_token(SerializationStrategy strategy, token_t token);

  void decode_using_strategy(SerializationStrategy strategy);

  void encode_using_strategy(SerializationStrategy strategy);

  void encode_adaptive();

  // debug compare function, can be called after encoding and decoding took
  // place to check if the original data matches the decoded
  void compare_encoded_decoded();

  std::vector<uint8_t>& get_data();

  public:
  std::array<std::vector<uint8_t>, N_STRATEGIES> m_data;
  std::array<std::vector<token_t>, N_STRATEGIES> m_tokens;
  std::array<StrategyResult, N_STRATEGIES> m_strategy_results;
  std::array<uint8_t, N_STRATEGIES> m_delta_params;
  uint16_t m_width;
  uint16_t m_height;
  std::vector<uint8_t> m_decoded_data;

  SerializationStrategy m_picked_strategy;
};

#endif  // LZ_CODEC_HPP