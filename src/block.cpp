#include "block.hpp"

#include <algorithm>
#include <algorithm>  // For std::find, std::rotate
#include <cstdint>    // For uint8_t
#include <iostream>
#include <iterator>  // For std::distance
#include <map>       // Assuming m_data is map-like (or similar structure)
#include <numeric>   // For std::iota
#include <stdexcept>
#include <stdexcept>  // For error handling
#include <vector>

#include "common.hpp"
#include "hashtable.hpp"
// #include "linkedlist.hpp"

Block::Block(const std::vector<uint8_t> data, uint16_t width, uint16_t height)
    : m_width(width), m_height(height), m_picked_strategy(HORIZONTAL) {
  for (size_t i = 0; i < N_STRATEGIES; i++) {
    m_data[i].reserve(width * height);
  }
  m_data[HORIZONTAL].assign(data.begin(), data.end());
  m_strategy_results.fill({0, 0});
}

Block::Block(uint16_t width, uint16_t height, SerializationStrategy strategy)
    : m_width(width), m_height(height), m_picked_strategy(strategy) {
}

void Block::serialize_all_strategies() {
  serialize(HORIZONTAL);
  serialize(VERTICAL);
  // serialize(DIAGONAL);
}

void Block::serialize(SerializationStrategy strategy) {
  switch (strategy) {
    case HORIZONTAL:
      // default, does not need to be serialized
      return;
    case VERTICAL:
      m_data[VERTICAL].clear();
      m_data[VERTICAL].reserve(m_width * m_height);
      for (size_t j = 0; j < m_width; ++j) {
        for (size_t i = 0; i < m_height; ++i) {
          m_data[VERTICAL].push_back(m_data[HORIZONTAL][i * m_width + j]);
        }
      }
      break;
    default:
      break;
  }
}

void Block::deserialize() {
  const SerializationStrategy strategy = m_picked_strategy;
  if (strategy == HORIZONTAL) {
    return;
  }

  m_decoded_deserialized_data.clear();
  m_decoded_deserialized_data.resize(m_width * m_height);

  for (size_t i = 0; i < m_height; ++i) {
    for (size_t j = 0; j < m_width; ++j) {
      size_t src_index = j * m_height + i;
      size_t dest_index = i * m_width + j;
      if (src_index >= m_decoded_data.size()) {
        throw std::runtime_error(
            "Deserialize error: Source index out of bounds.");
      }

      m_decoded_deserialized_data[dest_index] = m_decoded_data[src_index];
    }
  }
  m_decoded_data.clear();
  m_decoded_data.shrink_to_fit();
}

#if 1
void Block::delta_transform(SerializationStrategy strategy) {
  if (m_data.empty()) {
    return;
  }
  uint8_t prev_original = m_data[strategy][0];
  for (uint64_t i = 1; i < m_data[strategy].size(); i++) {
    uint8_t current_original = m_data[strategy][i];
    m_data[strategy][i] =
        static_cast<uint8_t>(current_original - prev_original);
    prev_original = current_original;
  }
}
void Block::reverse_delta_transform() {
  if (m_decoded_data.empty()) {
    return;
  }
  uint8_t prev_reconstructed = m_decoded_data[0];
  for (uint64_t i = 1; i < m_decoded_data.size(); i++) {
    m_decoded_data[i] =
        static_cast<uint8_t>(m_decoded_data[i] + prev_reconstructed);
    prev_reconstructed = m_decoded_data[i];
  }
}

#else
void Block::delta_transform(SerializationStrategy strategy) {
  // Convert the enum strategy to its underlying size_t index
  size_t strategy_index = static_cast<size_t>(strategy);

  // Runtime bounds check for safety (optional but recommended)
  // This prevents accessing the array out of bounds if an invalid
  // enum value (e.g., from an unsafe cast) is somehow passed.
  if (strategy_index >= N_STRATEGIES) {
    throw std::out_of_range(
        "Invalid SerializationStrategy value provided to delta_transform.");
  }

  // Get a reference to the specific vector using the strategy's index
  std::vector<uint8_t>& data_vec = m_data[strategy_index];

  // --- The core MTF logic remains unchanged ---

  // 1. Initialize the dictionary (0..255)
  std::vector<uint8_t> dictionary(256);
  std::iota(dictionary.begin(), dictionary.end(), 0);

  // 2. Iterate over the target data vector
  for (size_t i = 0; i < data_vec.size(); ++i) {
    uint8_t current_byte = data_vec[i];

    // 3. Find the byte in the dictionary
    auto dict_it =
        std::find(dictionary.begin(), dictionary.end(), current_byte);

    // This check should ideally never fail with valid byte data
    if (dict_it == dictionary.end()) {
      throw std::logic_error(
          "MTF Error: Byte value not found in the 0-255 dictionary. "
          "Indicates a potential data corruption or logic issue.");
    }

    // 4. Calculate the index
    uint8_t index =
        static_cast<uint8_t>(std::distance(dictionary.begin(), dict_it));

    // 5. Replace data byte with its current index
    data_vec[i] = index;

    // 6. Move the found byte to the front of the dictionary if needed
    if (index != 0) {
      std::rotate(dictionary.begin(), dict_it, dict_it + 1);
    }
  }
  // The 'dictionary' vector goes out of scope here, and its memory
  // is automatically managed (no leaks).
}

void Block::reverse_delta_transform() {
  // Get a reference to the specific vector containing the encoded indices
  std::vector<uint8_t>& encoded_data = m_decoded_data;

  // 1. Initialize the dictionary exactly as in the encoder
  std::vector<uint8_t> dictionary(256);
  std::iota(dictionary.begin(), dictionary.end(),
            0);  // Fills with 0, 1, ..., 255

  // 2. Iterate over the encoded data (which contains indices)
  for (size_t i = 0; i < encoded_data.size(); ++i) {
    uint8_t current_index = encoded_data[i];

    // 3. Check if the index is valid (within the dictionary bounds)
    //    An invalid index indicates corrupted data or an encoding error.
    if (current_index >= dictionary.size()) {  // size is always 256 here
      throw std::runtime_error(
          "MTF Decode Error: Invalid index encountered in encoded data.");
    }

    // 4. Look up the byte corresponding to the current index in the dictionary
    uint8_t decoded_byte = dictionary[current_index];

    // 5. Replace the index in the data vector with the decoded byte
    encoded_data[i] = decoded_byte;

    // 6. Update the dictionary state *exactly* like the encoder did:
    //    Move the `decoded_byte` (which was at `current_index`) to the front.
    //    Only perform the rotation if the element wasn't already at the front.
    if (current_index != 0) {
      // Find the iterator pointing to the element we just decoded
      // (which is currently at dictionary[current_index])
      auto dict_it = dictionary.begin() + current_index;

      // Rotate the element at dict_it to the beginning
      std::rotate(dictionary.begin(), dict_it, dict_it + 1);
    }
    // No else needed: if index was 0, the byte was already at the front.
  }
  // Dictionary goes out of scope, memory managed automatically.
}
#endif

void Block::insert_token(SerializationStrategy strategy, token_t token) {
#if 0
      std::cout << "insert_token: " << std::endl;
      std::cout << "  strategy: " << strategy << std::endl;
      std::cout << "  token: ";
      if (token.coded) {
        std::cout << "coded: " << static_cast<int>(token.data.offset) << " "
                  << static_cast<int>(token.data.length) << std::endl;
      } else {
        std::cout << "uncoded: " << static_cast<int>(token.data.value) << "("
                  << static_cast<char>(token.data.value) << ")" << std::endl;
      }
#endif
  m_tokens[strategy].push_back(token);
  // add to the strategy result for picking the best one in adaptive
  token.coded ? m_strategy_results[strategy].n_coded_tokens++
              : m_strategy_results[strategy].n_unencoded_tokens++;
}

void Block::decode_using_strategy(SerializationStrategy strategy) {
  if (strategy == DEFAULT) {
    strategy = m_picked_strategy;
  }
  auto& tokens = m_tokens[strategy];
  uint64_t position = 0;
  for (size_t i = 0; i < tokens.size(); i++) {
    token_t token = tokens[i];
    if (token.coded) {
      // coded token
      uint64_t token_position = position - token.data.offset;
      uint16_t length = token.data.length + MIN_CODED_LEN;
      for (uint16_t j = 0; j < length; j++) {
        m_decoded_data.push_back(m_decoded_data[token_position + j]);
      }
#if DEBUG_PRINT
      std::cout << "decoded: " << static_cast<int>(token.data.offset) << " "
                << static_cast<int>(token.data.length) << std::endl;
      std::cout << "decoded: ";
      for (uint16_t j = token_position; j < token_position + length; j++) {
        std::cout << static_cast<int>(m_decoded_data[j]) << " ";
      }
      std::cout << "(";
      for (uint16_t j = 0; j < length; j++) {
        std::cout << static_cast<char>(m_decoded_data[j]);
      }
      std::cout << ")" << std::endl;
#endif
      position += length;

    } else {
      // uncoded token
      m_decoded_data.push_back(token.data.value);
#if DEBUG_PRINT
      std::cout << "decoded: " << static_cast<int>(token.data.value) << "("
                << static_cast<char>(token.data.value) << ")" << std::endl;
#endif
      position++;
    }
  }
#if DEBUG_PRINT
  std::cout << "decoded data: ";
  for (size_t i = 0; i < m_decoded_data.size(); i++) {
    std::cout << static_cast<int>(m_decoded_data[i]) << " ";
  }
  std::cout << std::endl;
  std::cout << "decoded data: \t";
  for (size_t i = 0; i < m_decoded_data.size(); i++) {
    std::cout << static_cast<char>(m_decoded_data[i]);
  }
  std::cout << std::endl;
#endif
}

void Block::encode_using_strategy(SerializationStrategy strategy) {
  // if the strategy is not set, use the horizontal one
  if (strategy == DEFAULT) {
    strategy = HORIZONTAL;
  }

  auto hash_table = HashTable(HASH_TABLE_SIZE);
  // push the first two bytes unencoded since the dict is empty
  uint64_t position = 0;
  for (position = 0; position < MIN_CODED_LEN; position++) {
    insert_token(strategy, {.coded = false,
                            .data = {.value = m_data[strategy][position]}});
  }

  hash_table.insert(m_data[strategy], 0);
  uint64_t next_pos;
  uint64_t removed_until = 0;
  // iterate over all bytes of the input
  for (position = MIN_CODED_LEN, next_pos = MIN_CODED_LEN;
       position < m_data[strategy].size();) {
    // search for the longest prefix in the hash table
    search_result result = hash_table.search(m_data[strategy], position);
    next_pos = position + result.length;
    if (result.found) {
      next_pos += MIN_CODED_LEN;
      // found a match, push the token
      insert_token(
          strategy,
          {.coded = true,
           .data = {.offset = static_cast<uint16_t>(position - result.position),
                    .length = result.length}});
    } else {
      // no match found, push the byte unencoded
      insert_token(strategy, {.coded = false,
                              .data = {.value = m_data[strategy][position]}});
      next_pos++;
    }

    // insert new prefixes into the hash table
    while (position < next_pos) {
      hash_table.insert(m_data[strategy], position - MIN_CODED_LEN + 1);
      position++;
    }

    // remove old entries from the hash table
    if (position > SEARCH_BUF_SIZE) {
      size_t remove_from = removed_until;
      size_t remove_to = position - SEARCH_BUF_SIZE - 1;
      for (size_t r = remove_from; r <= remove_to; r++) {
        hash_table.remove(m_data[strategy], r);
      }
      removed_until = remove_to + 1;
    }
  }
}

void Block::encode_adaptive() {
  size_t best_encoded_size = 0, current_strategy_result;
  bool first = true;
  for (size_t i = HORIZONTAL; i < N_STRATEGIES; i++) {
    // delta_transform(true);
    encode_using_strategy(static_cast<SerializationStrategy>(i));
    current_strategy_result =
        m_strategy_results[i].n_coded_tokens * TOKEN_CODED_LEN +
        m_strategy_results[i].n_unencoded_tokens * TOKEN_UNCODED_LEN;
    if (first || current_strategy_result < best_encoded_size) {
      best_encoded_size = current_strategy_result;
      m_picked_strategy = static_cast<SerializationStrategy>(i);
      first = false;
    } else {
      // if the current strategy is not the best one, clear the tokens
      m_tokens[i].clear();
    }
  }
}

// debug compare function, can be called after encoding and decoding took
// place to check if the original data matches the decoded
void Block::compare_encoded_decoded() {
  bool identical = true;
  for (size_t i = 0; i < m_data[m_picked_strategy].size(); i++) {
    if (m_decoded_data[i] != m_data[m_picked_strategy][i]) {
      std::cout << "Error: Decoded data does not match original data at index "
                << i << ": " << static_cast<int>(m_decoded_data[i])
                << " != " << static_cast<int>(m_data[m_picked_strategy][i])
                << std::endl;
      identical = false;
    }
  }
  if (!identical) {
    std::cout << "Decoded data does not match original data." << std::endl;
    throw std::runtime_error(
        "Error: Decoded data does not match original data.");
  }
#if DEBUG_PRINT
  else {
    std::cout << "Decoded data matches original data." << std::endl;
  }
#endif
}

void Block::print_tokens() {
  std::cout << "Tokens for strategy: " << m_picked_strategy << std::endl;
  std::cout << "------------------------------" << std::endl;
  std::cout << "Total tokens: " << m_tokens[m_picked_strategy].size()
            << std::endl;
  for (auto& token : m_tokens[m_picked_strategy]) {
    if (token.coded) {
      std::cout << "<1, " << static_cast<int>(token.data.offset) << ", "
                << static_cast<int>(token.data.length) << ">" << std::endl;
    } else {
      std::cout << "<0, " << static_cast<int>(token.data.value) << ">"
                << std::endl;
    }
  }
}

std::vector<uint8_t>& Block::get_data() {
  return m_data[m_picked_strategy];
}
