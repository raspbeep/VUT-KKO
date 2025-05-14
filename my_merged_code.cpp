// src/argparser.cpp
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

#include <argparse.hpp>
#include <iostream>

#include "argparser.hpp"
#include "common.hpp"

ArgumentParser::ArgumentParser(int argc, char* argv[])
    : program("lz_codec", "1.0", argparse::default_arguments::help) {
  auto& group = program.add_mutually_exclusive_group();
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
    // print_args();
  } catch (const std::runtime_error& err) {
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

// src/argparser.hpp
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
  ArgumentParser(int argc, char* argv[]);

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

// src/block.cpp
/**
 * @file      block.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Block class implementation for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "block.hpp"
#include "common.hpp"
#include "hashtable.hpp"

Block::Block(const std::vector<uint8_t> data, uint32_t width, uint32_t height)
    : m_width(width), m_height(height), m_picked_strategy(HORIZONTAL) {
  for (size_t i = 0; i < N_STRATEGIES; i++) {
    m_data[i].reserve(width * height);
  }
  m_data[HORIZONTAL].assign(data.begin(), data.end());
  m_strategy_results.fill({0, 0});
}

Block::Block(uint32_t width, uint32_t height, SerializationStrategy strategy)
    : m_width(width), m_height(height), m_picked_strategy(strategy) {
}

void Block::serialize_all_strategies() {
  serialize(HORIZONTAL);
  serialize(VERTICAL);
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
    size_t dest_index_no_j = i * m_width;
    for (size_t j = 0; j < m_width; ++j) {
      size_t src_index = j * m_height + i;
      if (__builtin_expect(src_index >= m_decoded_data.size(), 0)) {
        throw std::runtime_error(
            "Deserialize error: Source index out of bounds.");
      }

      m_decoded_deserialized_data[dest_index_no_j + j] =
          m_decoded_data[src_index];
    }
  }
  // m_decoded_data.clear();
  // m_decoded_data.shrink_to_fit();
}

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

void Block::mtf(SerializationStrategy strategy) {
  size_t strategy_index = static_cast<size_t>(strategy);

  std::vector<uint8_t>& data_vec = m_data[strategy_index];

  std::vector<uint8_t> dictionary(256);
  std::iota(dictionary.begin(), dictionary.end(), 0);

  for (size_t i = 0; i < data_vec.size(); ++i) {
    uint8_t current_byte = data_vec[i];

    auto dict_it =
        std::find(dictionary.begin(), dictionary.end(), current_byte);

    if (dict_it == dictionary.end()) {
      throw std::logic_error(
          "MTF Error: Byte value not found in the 0-255 dictionary. "
          "Indicates a potential data corruption.");
    }

    uint8_t index =
        static_cast<uint8_t>(std::distance(dictionary.begin(), dict_it));

    data_vec[i] = index;

    if (index != 0) {
      std::rotate(dictionary.begin(), dict_it, dict_it + 1);
    }
  }
}

void Block::reverse_mtf() {
  std::vector<uint8_t>& encoded_data = m_decoded_data;

  std::vector<uint8_t> dictionary(256);
  std::iota(dictionary.begin(), dictionary.end(), 0);

  for (size_t i = 0; i < encoded_data.size(); ++i) {
    uint8_t current_index = encoded_data[i];

    if (current_index >= dictionary.size()) {
      throw std::runtime_error(
          "MTF Decode Error: Invalid index encountered in encoded data.");
    }

    uint8_t decoded_byte = dictionary[current_index];

    encoded_data[i] = decoded_byte;
    if (current_index != 0) {
      auto dict_it = dictionary.begin() + current_index;

      std::rotate(dictionary.begin(), dict_it, dict_it + 1);
    }
  }
}

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
  m_decoded_data.reserve(m_width * m_height);
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

// void Block::encode_using_strategy(SerializationStrategy strategy) {
//   // if the strategy is not set, use the horizontal one
//   if (strategy == DEFAULT) {
//     strategy = HORIZONTAL;
//   }

//   auto hash_table = HashTable(HASH_TABLE_SIZE);
//   // push the first two bytes unencoded since the dict is empty
//   uint64_t position = 0;
//   for (position = 0; position < MIN_CODED_LEN; position++) {
//     insert_token(strategy, {.coded = false,
//                             .data = {.value = m_data[strategy][position]}});
//   }

//   hash_table.insert(m_data[strategy], 0);
//   uint64_t next_pos;
//   uint64_t removed_until = 0;
//   // iterate over all bytes of the input
//   for (position = MIN_CODED_LEN, next_pos = MIN_CODED_LEN;
//        position < m_data[strategy].size();) {
//     // search for the longest prefix in the hash table
//     search_result result = hash_table.search(m_data[strategy], position);
//     next_pos = position + result.length;

//     if (__builtin_expect(result.found, 1)) {
//       next_pos += MIN_CODED_LEN;
//       // found a match, push the token
//       insert_token(
//           strategy,
//           {.coded = true,
//            .data = {.offset = static_cast<uint16_t>(position -
//            result.position),
//                     .length = result.length}});
//     } else {
//       // no match found, push the byte unencoded
//       insert_token(strategy, {.coded = false,
//                               .data = {.value =
//                               m_data[strategy][position]}});
//       next_pos++;
//     }

//     // insert new prefixes into the hash table
//     while (position < next_pos) {
//       hash_table.insert(m_data[strategy], position - MIN_CODED_LEN + 1);
//       position++;
//     }

//     // remove old entries from the hash table
//     if (position > SEARCH_BUF_SIZE) {
//       size_t remove_from = removed_until;
//       size_t remove_to = position - SEARCH_BUF_SIZE - 1;
//       for (size_t r = remove_from; r <= remove_to; r++) {
//         hash_table.remove(m_data[strategy], r);
//       }
//       removed_until = remove_to + 1;
//     }
//   }
// }

const uint64_t N_ITERATIONS_TO_RESET_HASHTABLE = 1000;

void Block::encode_using_strategy(SerializationStrategy strategy) {
  if (strategy == DEFAULT) {
    strategy = HORIZONTAL;
  }

  auto hash_table = HashTable(HASH_TABLE_SIZE);
  uint64_t position = 0;

  // Handle cases where data size is less than MIN_CODED_LEN
  // This part ensures we don't access out of bounds if m_data[strategy] is too
  // small.
  if (m_data[strategy].size() < MIN_CODED_LEN) {
    for (position = 0; position < m_data[strategy].size(); ++position) {
      insert_token(strategy, {.coded = false,
                              .data = {.value = m_data[strategy][position]}});
    }
    return;  // Nothing more to encode
  }

  // Process the first MIN_CODED_LEN bytes as unencoded literals,
  // as there's not enough history for them to be coded.
  for (position = 0; position < MIN_CODED_LEN; ++position) {
    insert_token(strategy, {.coded = false,
                            .data = {.value = m_data[strategy][position]}});
  }

  // Insert the first actual sequence into the hash table.
  // This sequence starts at index 0 and has MIN_CODED_LEN bytes.
  // This call was `hash_table.insert(m_data[strategy], 0);` in the original
  // snippet. It primes the hash table with the very first sequence.
  if (MIN_CODED_LEN > 0 && m_data[strategy].size() >= MIN_CODED_LEN) {
    hash_table.insert(m_data[strategy], 0);
  }

  uint64_t removed_until = 0;
  uint64_t iterations_since_last_reset = 0;  // Counter for N iterations

  // The main loop starts processing from 'position' (which is now
  // MIN_CODED_LEN). 'position' is the start of the current lookahead buffer.
  // The loop variable 'position' is advanced inside the loop based on match
  // length.
  for (/* position is already MIN_CODED_LEN */;
       position < m_data[strategy].size();
       /* position updated inside */) {
    // Check if it's time to reset the HashTable
    if (N_ITERATIONS_TO_RESET_HASHTABLE > 0 &&
        iterations_since_last_reset >= N_ITERATIONS_TO_RESET_HASHTABLE) {
      hash_table = HashTable(HASH_TABLE_SIZE);  // Re-initialize the HashTable
      iterations_since_last_reset = 0;          // Reset the iteration counter

      // When the HashTable is reset, 'removed_until' must also be reset.
      // Setting it to 'position' ensures that the 'remove old entries' logic
      // (which loops from 'removed_until' to 'position - SEARCH_BUF_SIZE - 1')
      // will not attempt to remove entries from the new, empty HashTable
      // that were never inserted into it. The loop condition
      // 'r <= position - SEARCH_BUF_SIZE - 1' with 'r' starting at 'position'
      // means the loop won't execute initially, which is safe.
      removed_until = position;

      // After reset, the hash table is empty. To maintain compression quality,
      // it's beneficial to "re-prime" it with recent history that falls
      // within the current sliding window.
      // We insert sequences ending from `position - 1` down to `position -
      // (SEARCH_BUF_SIZE - MIN_CODED_LEN +1)` (or as many as are available and
      // fit the MIN_CODED_LEN requirement). This ensures that the search
      // immediately after a reset can find recent patterns. Iterate backwards
      // from the byte just before the current 'position'. The sequences
      // inserted are those whose *last* byte is `idx_last_byte`. The start of
      // such a sequence is `idx_last_byte - MIN_CODED_LEN + 1`.
      uint64_t lookback_limit =
          (position > SEARCH_BUF_SIZE) ? (position - SEARCH_BUF_SIZE) : 0;
      for (uint64_t p_reprime = position;
           p_reprime > lookback_limit && p_reprime >= MIN_CODED_LEN - 1;
           --p_reprime) {
        uint64_t seq_start_idx = p_reprime - (MIN_CODED_LEN - 1);
        if (seq_start_idx + MIN_CODED_LEN <=
            m_data[strategy].size()) {  // Ensure valid sequence
          // Avoid re-inserting the very first sequence if it was just done
          // before the loop
          if (seq_start_idx == 0 && position == MIN_CODED_LEN)
            continue;
          hash_table.insert(m_data[strategy], seq_start_idx);
        }
        if (p_reprime == 0)
          break;  // avoid underflow if p_reprime is unsigned
      }
    }
    iterations_since_last_reset++;  // Increment per token processed

    search_result result = hash_table.search(m_data[strategy], position);
    uint64_t position_after_this_token;

    if (__builtin_expect(result.found, 1)) {
      insert_token(
          strategy,
          {.coded = true,
           .data = {.offset = static_cast<uint16_t>(position - result.position),
                    .length = result.length}});
      position_after_this_token = position + result.length + MIN_CODED_LEN;
    } else {
      insert_token(strategy, {.coded = false,
                              .data = {.value = m_data[strategy][position]}});
      position_after_this_token = position + 1;
    }

    // Insert new prefixes into the hash table.
    // These are sequences whose *last* byte is at `p_insert_iter - 1` (if
    // MIN_CODED_LEN > 0), so they start at `(p_insert_iter - 1) - MIN_CODED_LEN
    // + 1`. The loop iterates from the current `position` up to
    // `position_after_this_token`. For each byte
    // `m_data[strategy][idx_last_byte]` that moves from lookahead to history,
    // insert the sequence `m_data[strategy][idx_last_byte - MIN_CODED_LEN + 1
    // ... idx_last_byte]`.
    uint64_t current_byte_idx_for_insertion_loop = position;
    while (current_byte_idx_for_insertion_loop < position_after_this_token) {
      // Check if there are enough preceding characters to form a sequence of
      // MIN_CODED_LEN
      if (current_byte_idx_for_insertion_loop >= MIN_CODED_LEN - 1) {
        uint64_t start_of_sequence_to_insert =
            current_byte_idx_for_insertion_loop - (MIN_CODED_LEN - 1);
        // Ensure this sequence is within data bounds before inserting
        if (start_of_sequence_to_insert + MIN_CODED_LEN <=
            m_data[strategy].size()) {
          hash_table.insert(m_data[strategy], start_of_sequence_to_insert);
        }
      }
      current_byte_idx_for_insertion_loop++;
    }

    // Update main loop position variable
    position = position_after_this_token;

    // remove old entries from the hash table
    // 'position' is now the start of the *next* lookahead buffer.
    // We remove sequences that are too far in the past relative to this new
    // 'position'. A sequence starting at 'r' is too old if 'position - r >
    // SEARCH_BUF_SIZE'. So, r < position - SEARCH_BUF_SIZE. The oldest sequence
    // to keep starts at 'position - SEARCH_BUF_SIZE'. Thus, we remove sequences
    // starting from 'removed_until' up to 'position - SEARCH_BUF_SIZE - 1'.
    if (position > SEARCH_BUF_SIZE) {  // Check if the lookahead has moved past
                                       // the window size
      size_t remove_to_idx = position - SEARCH_BUF_SIZE - 1;
      // Ensure remove_to_idx is not less than removed_until to have a valid
      // loop range. Also, removed_until should not cause r_idx to be negative
      // if it's unsigned.
      if (remove_to_idx >= removed_until &&
          remove_to_idx <
              m_data[strategy].size()) {  // Check remove_to_idx validity
        for (size_t r_idx = removed_until; r_idx <= remove_to_idx; ++r_idx) {
          // Ensure the sequence to remove is validly formed
          if (r_idx + MIN_CODED_LEN <= m_data[strategy].size()) {
            hash_table.remove(m_data[strategy], r_idx);
          }
        }
        removed_until = remove_to_idx + 1;
      } else if (position > removed_until + SEARCH_BUF_SIZE) {
        // This case can occur if position jumped very far, past the current
        // window from removed_until or if removed_until was set to 'position'
        // during a reset. We need to update removed_until to the new valid
        // start.
        removed_until =
            (position > SEARCH_BUF_SIZE) ? (position - SEARCH_BUF_SIZE) : 0;
      }
    }
    // If position is now at or beyond the data size, the loop will terminate.
    if (position >= m_data[strategy].size()) {
      break;
    }
  }
}

void Block::encode_adaptive() {
  size_t best_encoded_size = 0, current_strategy_result;
  bool first = true;
  for (size_t i = HORIZONTAL; i < N_STRATEGIES; i++) {
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
// triggered by enabling DEBUG_COMP_ENC_UNENC
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

// src/block.hpp
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

// src/block_reader.cpp
/**
 * @file      block_reader.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Block reader implementation for bit packed decompression output
 *
 * @date      12 April  2025 \n
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "block.hpp"
#include "token.hpp"

// internal state for bit reading
uint8_t reader_buffer = 0;
int reader_bit_position = 8;
bool reader_eof = false;

// resets the internal state of the bit reader
void reset_bit_reader_state() {
  reader_buffer = 0;
  reader_bit_position = 8;
  reader_eof = false;
}

bool read_bit_from_file(std::ifstream& file, bool& bitValue) {
  if (reader_eof) {
    return false;
  }

  if (reader_bit_position == 8) {
    if (!file.read(reinterpret_cast<char*>(&reader_buffer), 1)) {
      reader_eof = file.eof();
      return false;
    }
    reader_bit_position = 0;
  }

  // extract the bit
  bitValue = (reader_buffer >> (7 - reader_bit_position)) & 1;
  reader_bit_position++;

  return true;
}

bool read_bits_from_file(std::ifstream& file, int numBits, uint32_t& value) {
  if (numBits < 0 || numBits > 32) {
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  if (reader_eof)
    return false;

  value = 0;
  for (int i = 0; i < numBits; i++) {
    bool current_bit;
    if (!read_bit_from_file(file, current_bit)) {
      return false;
    }
    value = (value << 1) | (current_bit ? 1 : 0);
  }
  return true;
}

bool read_blocks_from_file(const std::string& filename, uint32_t& width,
                           uint32_t& height, uint16_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks, bool& binary_only) {
  blocks.clear();
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for reading: " << filename << std::endl;
    reset_bit_reader_state();
    return false;
  }

  reset_bit_reader_state();
  uint8_t successful_compression;
  try {
    // read header not bit-packed for consistency
    file.read(reinterpret_cast<char*>(&successful_compression),
              sizeof(successful_compression));
    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    file.read(reinterpret_cast<char*>(&height), sizeof(height));
    file.read(reinterpret_cast<char*>(&offset_bits), sizeof(offset_bits));
    file.read(reinterpret_cast<char*>(&length_bits), sizeof(length_bits));

    if (!file.good()) {
      throw std::runtime_error("Failed to read file header.");
    }

    if (!read_bit_from_file(file, model)) {
      throw std::runtime_error("Failed to read model flag.");
    }

    if (!read_bit_from_file(file, adaptive)) {
      throw std::runtime_error("Failed to read adaptive flag.");
    }

    if (!read_bit_from_file(file, binary_only)) {
      throw std::runtime_error("Failed to read adaptive flag.");
    }

    if (adaptive) {
      uint32_t temp_block_size;
      if (!read_bits_from_file(file, 16, temp_block_size)) {
        throw std::runtime_error(
            "Failed to read block size for adaptive mode.");
      }

      if (temp_block_size > UINT16_MAX) {
        throw std::runtime_error(
            "Block size read from file exceeds uint16_t max.");
      }
      BLOCK_SIZE = static_cast<uint16_t>(temp_block_size);
      if (BLOCK_SIZE == 0) {
        throw std::runtime_error("Adaptive mode read invalid block size (0).");
      }
    }

    uint16_t n_row_blocks = 1;
    uint16_t n_col_blocks = 1;
    if (adaptive) {
      if (BLOCK_SIZE == 0) {
        throw std::runtime_error(
            "Cannot calculate blocks: Adaptive mode but block size is zero.");
      }
      n_row_blocks = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
      n_col_blocks = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    for (uint16_t row = 0; row < n_row_blocks; row++) {
      for (uint16_t col = 0; col < n_col_blocks; col++) {
        // calculate the current block's width and height
        uint32_t current_block_width = width;
        uint32_t current_block_height = height;
        if (adaptive) {
          current_block_width =
              std::min<uint32_t>(BLOCK_SIZE, width - col * BLOCK_SIZE);
          current_block_height =
              std::min<uint32_t>(BLOCK_SIZE, height - row * BLOCK_SIZE);
        }

        uint64_t expected_decoded_bytes =
            static_cast<uint64_t>(current_block_width) * current_block_height;

        // read the strategy from the file
        uint32_t strategy_val = DEFAULT;
        if (adaptive) {
          if (!read_bits_from_file(file, 2, strategy_val)) {
            std::cerr << "Warning: EOF or read error encountered while reading "
                         "strategy for block ("
                      << row << "," << col << ")." << std::endl;
            throw std::runtime_error(
                "Failed to read strategy for block. Possible EOF or read "
                "error.");
          }
        }

        if (strategy_val >= N_STRATEGIES) {
          std::cerr << "Error: Invalid strategy value read from file: "
                    << strategy_val << " for block (" << row << "," << col
                    << ")." << std::endl;
          reset_bit_reader_state();
          return false;
        }

        SerializationStrategy strategy =
            static_cast<SerializationStrategy>(strategy_val);

        Block block(current_block_width, current_block_height, strategy);
        uint64_t total_decoded_bytes_for_block = 0;
        while (total_decoded_bytes_for_block < expected_decoded_bytes) {
          // read tokens for the block
          token_t token;
          bool flag_bit;
          // read coded flag (1 bit)
          if (!read_bit_from_file(file, flag_bit)) {
            // EOF hit unexpectedly before the block was fully decoded
            std::cerr << "Warning: Unexpected EOF encountered while reading "
                         "token flag for block ("
                      << row << "," << col << ")."
                      << " Expected " << expected_decoded_bytes
                      << " bytes, got " << total_decoded_bytes_for_block << "."
                      << std::endl;
            goto end_reading;  // Exit loops
          }
          token.coded = flag_bit;

          // read token data
          if (token.coded) {
            uint32_t temp_offset, temp_length;
            // Coded token: read offset and length
            if (!read_bits_from_file(file, offset_bits, temp_offset)) {
              std::cerr << "Warning: EOF encountered while reading offset for "
                           "coded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.offset = static_cast<uint16_t>(temp_offset);

            if (!read_bits_from_file(file, length_bits, temp_length)) {
              std::cerr << "Warning: EOF encountered while reading length for "
                           "coded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.length = static_cast<uint16_t>(temp_length);
            // increment decoded bytes count
            total_decoded_bytes_for_block +=
                (token.data.length + MIN_CODED_LEN);
          } else {
            uint32_t temp_value;
            // uncoded token: read ASCII value (8 bits)
            if (!read_bits_from_file(file, 8, temp_value)) {
              std::cerr << "Warning: EOF encountered while reading value for "
                           "uncoded token in block ("
                        << row << "," << col << ")." << std::endl;
              goto end_reading;
            }
            token.data.value = static_cast<uint8_t>(temp_value);
            // increment decoded bytes count
            total_decoded_bytes_for_block++;
          }

          // if we successfully read all parts of the token, add it
          block.m_tokens[strategy].push_back(token);
        }

        // sanity check after loop
        if (total_decoded_bytes_for_block != expected_decoded_bytes) {
          std::cerr << "Warning: Block (" << row << "," << col
                    << ") decoding finished, but byte count mismatch. Expected "
                    << expected_decoded_bytes << ", got "
                    << total_decoded_bytes_for_block << "." << std::endl;
        }

        blocks.push_back(std::move(block));
      }
    }

  } catch (const std::exception& e) {
    std::cerr << "Error during file reading: " << e.what() << std::endl;
    reset_bit_reader_state();  // reset state on error
    file.close();
    return false;
  }

end_reading:

  // reset state after successful read or normal EOF break
  reset_bit_reader_state();
  file.close();

  return true;
}

void printTokens(const std::vector<token_t>& tokens) {
  std::cout << "Total tokens: " << tokens.size() << std::endl;
  std::cout << "------------------------------" << std::endl;

  for (size_t i = 0; i < tokens.size(); i++) {
    const auto& token = tokens[i];

    std::cout << "Token " << std::setw(3) << i << ": ";
    if (token.coded) {
      std::cout << "CODED   - Offset: " << std::setw(4) << token.data.offset
                << ", Length: " << std::setw(2) << token.data.length
                << std::endl;
    } else {
      char c = static_cast<char>(token.data.value);
      std::cout << "UNCODED - ASCII: " << std::setw(3)
                << static_cast<int>(token.data.value);
      if (token.data.value >= 32 && token.data.value <= 126) {
        std::cout << " ('" << c << "')";
      }
      std::cout << std::endl;
    }
  }
}

// src/block_reader.hpp
/**
 * @file      block_reader.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the block reader implementation for bit packed
 * decompression output
 *
 * @date      12 April  2025 \n
 */

#ifndef BLOCK_READER_HPP
#define BLOCK_READER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "block.hpp"  // Include necessary header for Block class
#include "token.hpp"  // Include necessary header for token_t

/**
 * @brief Reads compressed data from a file, reconstructing header information
 * and tokenized blocks.
 * @param filename The path to the compressed input file.
 * @param width Output parameter for the width of the original data.
 * @param height Output parameter for the height of the original data.
 * @param offset_bits Output parameter for the number of bits used for offsets
 * in coded tokens.
 * @param length_bits Output parameter for the number of bits used for lengths
 * in coded tokens.
 * @param adaptive Output parameter indicating if adaptive mode was used during
 * compression.
 * @param model Output parameter indicating if model preprocessing was used
 * during compression.
 * @param blocks Output parameter, a vector to be filled with the reconstructed
 * Block objects containing tokens.
 * @return True if the file was read successfully and blocks were reconstructed,
 * false otherwise (e.g., file not found, read error, corrupted data).
 */
bool read_blocks_from_file(const std::string& filename, uint32_t& width,
                           uint32_t& height, uint16_t& offset_bits,
                           uint16_t& length_bits, bool& adaptive, bool& model,
                           std::vector<Block>& blocks, bool& binary_only);

#endif  // BLOCK_READER_HPP

// src/block_writer.cpp
/**
 * @file      block_writer.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Block writer implementation for bit packed compression output
 *
 * @date      12 April  2025 \n
 */

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "block.hpp"
#include "block_writer.hpp"
#include "token.hpp"

uint8_t writer_buffer = 0;
int writer_bit_count = 0;

void reset_bit_writer_state() {
  writer_buffer = 0;
  writer_bit_count = 0;
}

void write_bit_to_file(std::ofstream& file, bool bit) {
  if (!file.is_open() || !file.good()) {
    throw std::runtime_error("File stream is not valid for writing.");
  }

  writer_buffer = (writer_buffer << 1) | (bit ? 1 : 0);
  writer_bit_count++;

  if (writer_bit_count == 8) {
    file.write(reinterpret_cast<const char*>(&writer_buffer), 1);
    if (!file.good()) {
      throw std::runtime_error("Failed to write byte to file.");
    }
    writer_buffer = 0;
    writer_bit_count = 0;
  }
}

void write_bits_to_file(std::ofstream& file, uint32_t value, int numBits) {
  if (numBits < 0 || numBits > 32) {
    throw std::out_of_range("Number of bits must be between 0 and 32.");
  }
  for (int i = numBits - 1; i >= 0; i--) {
    write_bit_to_file(file, (value >> i) & 1);
  }
}

void flush_bits_to_file(std::ofstream& file) {
  if (!file.is_open() || !file.good()) {
    reset_bit_writer_state();
    return;
  }

  if (writer_bit_count > 0) {
    writer_buffer <<= (8 - writer_bit_count);
    file.write(reinterpret_cast<const char*>(&writer_buffer), 1);
    if (!file.good()) {
      std::cerr << "Warning: Failed to write final byte during flush."
                << std::endl;
    }
  }
  // reset state after flushing
  reset_bit_writer_state();
}

// function to write tokens to binary file with bit packing
bool write_blocks_to_stream(const std::string& filename, uint32_t width,
                            uint32_t height, uint16_t offset_length,
                            uint16_t length_bits, bool adaptive, bool model,
                            const std::vector<Block>& blocks,
                            bool binary_only) {
  std::ofstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file for writing: " << filename << std::endl;
    return false;
  }

  reset_bit_writer_state();
  uint8_t successful_compression = 1;
  try {
    file.write(reinterpret_cast<const char*>(&successful_compression),
               sizeof(successful_compression));
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    file.write(reinterpret_cast<const char*>(&offset_length),
               sizeof(offset_length));
    file.write(reinterpret_cast<const char*>(&length_bits),
               sizeof(length_bits));
    write_bit_to_file(file, model);
    write_bit_to_file(file, adaptive);
    write_bit_to_file(file, binary_only);
    if (adaptive) {
      std::cout << "Writing block size of " << BLOCK_SIZE << " to file."
                << std::endl;
      write_bits_to_file(file, BLOCK_SIZE, 16);
    }

    if (!file.good())
      throw std::runtime_error("Failed to write header.");

    for (const auto& block : blocks) {
      if (adaptive) {
        // write strategy as 2 bits
        write_bits_to_file(file, block.m_picked_strategy, 2);
      }

      // write tokens with bit packing using functional helpers
      for (const auto& token : block.m_tokens[block.m_picked_strategy]) {
        // Write coded flag (1 bit)
        write_bit_to_file(file, token.coded);

        // write token data
        if (token.coded) {
          // Coded token: write offset and length with specified bit lengths
          if (offset_length > 16 || length_bits > 16) {
            throw std::out_of_range(
                "Offset/Length bit size too large for uint16_t.");
          }
          write_bits_to_file(file, token.data.offset, offset_length);
          write_bits_to_file(file, token.data.length, length_bits);
        } else {
          // uncoded token: write ASCII value (8 bits)
          write_bits_to_file(file, token.data.value, 8);
        }
      }
    }

    // flush any remaining bits
    flush_bits_to_file(file);  // Also resets state

  } catch (const std::exception& e) {
    std::cerr << "Error during file writing: " << e.what() << std::endl;
    // ensure state is reset even on error before closing
    reset_bit_writer_state();
    file.close();
    return false;
  }

  file.close();
  std::cout << "File written successfully: " << filename << std::endl;
  return true;
}

// src/block_writer.hpp
/**
 * @file      block_writer.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for the block writer implementation for bit packed
 * compression output
 *
 * @date      12 April  2025 \n
 */

#ifndef BLOCK_WRITER_HPP
#define BLOCK_WRITER_HPP

#include <cstdint>  // Include necessary types
#include <string>   // Include necessary types
#include <vector>

#include "block.hpp"  // Include Block definition

/**
 * @brief Writes the compressed data, including header and tokenized blocks, to
 * a file using bit packing.
 * @param filename The path to the output file.
 * @param width The width of the original data.
 * @param height The height of the original data.
 * @param offset_bits The number of bits used for offsets in coded tokens.
 * @param length_bits The number of bits used for lengths in coded tokens.
 * @param adaptive Flag indicating if adaptive mode was used.
 * @param model Flag indicating if model preprocessing was used.
 * @param blocks A vector of Block objects containing the tokens to be written.
 * @return True if writing was successful, false otherwise (e.g., file error).
 */
bool write_blocks_to_stream(const std::string& filename, uint32_t width,
                            uint32_t height, uint16_t offset_bits,
                            uint16_t length_bits, bool adaptive, bool model,
                            const std::vector<Block>& blocks, bool binary_only);

#endif  // BLOCK_WRITER_HPP

// src/common.hpp
/**
 * @file      common.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Common header file for LZSS compression with configuration
 * parameters and default values
 *
 * @date      12 April  2025 \n
 */

#ifndef COMMON_HPP
#define COMMON_HPP

#include <cmath>
#include <cstdint>

// debug priting options
#define DEBUG_DUMMY_SEQ 0
#define DEBUG_PRINT 0
#define DEBUG_COMP_ENC_UNENC 0
#define DEBUG_PRINT_TOKENS 0
#define DEBUG_PRINT_COLLISIONS 0

#define BINARY_ONLY 1

// minimum encode length
#define MIN_CODED_LEN 3

// top 10 10
#define DEFAULT_BLOCK_SIZE 64
#define DEFAULT_OFFSET_BITS 15
#define DEFAULT_LENGTH_BITS 15

// use MTF, if 0 use delta transform
#define MTF 1

extern uint16_t SEARCH_BUF_SIZE;
extern uint16_t OFFSET_BITS;
extern uint16_t LENGTH_BITS;
extern uint16_t MAX_CODED_LEN;

// used only for statistics printing
extern size_t TOKEN_CODED_LEN;
extern size_t TOKEN_UNCODED_LEN;

struct StrategyResult {
  size_t n_coded_tokens;
  size_t n_unencoded_tokens;
};

extern uint16_t BLOCK_SIZE;

constexpr size_t HORIZONTAL = 0;
constexpr size_t VERTICAL = 1;
constexpr size_t N_STRATEGIES = 2;
constexpr size_t DEFAULT = HORIZONTAL;

using SerializationStrategy = std::size_t;

#endif  // COMMON_HPP

// src/hashtable.cpp
/**
 * @file      hashtable.cpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Hash table implementation for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#include <array>
#include <iostream>
#include <stdexcept>

#include "hashtable.hpp"

const uint32_t TABLE_MASK = HASH_TABLE_SIZE - 1;
uint16_t max_additional_length = (1U << LENGTH_BITS) - 1;
uint16_t optimisation_threshold = max_additional_length * 0.5;

uint32_t HashTable::hash_function(std::vector<uint8_t>& data,
                                  uint64_t position) {
  uint32_t k1 = 0;
  uint64_t end_position = position + MIN_CODED_LEN > data.size()
                              ? data.size()
                              : position + MIN_CODED_LEN;
  uint16_t shift_left = 0;
  for (uint64_t i = position; i < end_position; i++) {
    k1 |= static_cast<uint32_t>(data[i]) << shift_left;
    shift_left += 8;
  }

  // // simple mixing (Knuth's multiplicative hash constant)
  k1 *= 0x9E3779B9;
  k1 ^= k1 >> 16;

  // k1 ^= k1 >> 13;
  // k1 *= 0x5bd1e995;
  // k1 ^= k1 >> 15;

  // use bitwise AND to get index in the range of the hash table size
  return k1 & TABLE_MASK;
}

// allocates a hash table of size 'size' and initializes all entries to nullptr
HashTable::HashTable(uint32_t size) : size(size), collision_count(0) {
  table = new HashNode*[size];
  for (uint16_t i = 0; i < size; ++i) {
    table[i] = nullptr;
  }
}

// iterates over the hash table and deletes all LL nodes and the hash table
// itself
HashTable::~HashTable() {
#if DEBUG_PRINT_COLLISIONS
  std::cout << "Collision count: " << collision_count << std::endl;
#endif
  for (uint16_t i = 0; i < size; ++i) {
    HashNode* current = table[i];
    while (current != nullptr) {
      HashNode* temp = current;
      current = current->next;
      delete temp;
    }
  }
  delete[] table;
}

// finds the index of the hash table for the given content and returns the
// longes prefix in the input values
// returns a struct with the following values:
// - found: true if a match was found
// - position: position in the input stream
// - length: length of the match
// if no match was found, returns a struct with found = false
search_result HashTable::search(std::vector<uint8_t>& data,
                                uint64_t current_pos) {
  uint32_t key = hash_function(data, current_pos);  // hashes M bytes of data

  HashNode* current = table[key];
  struct search_result result{
      false,  // found
      0,      // position
      0,      // length
  };

  // traverse the linked list at the index of the hash table
  while (current != nullptr) {
    bool match = true;
    for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
      uint8_t cmp1 = data[current_pos + i];
      uint8_t cmp2 = data[current->position + i];
      if (__builtin_expect(cmp1 != cmp2, 0)) {
        // hash collision! hashes matched but the data is different
        collision_count++;
#if DEBUG_PRINT_COLLISIONS
        std::cout << "HashTable::search: hash collision!" << std::endl;
        std::cout << "string1: ";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<int>(data[current_pos + j]) << " ";
        }
        std::cout << "(";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<char>(data[current_pos + j]);
        }
        std::cout << ")" << std::endl;
        std::cout << "string2: ";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<int>(data[current->position + j]) << " ";
        }
        std::cout << "(";
        for (uint16_t j = 0; j < MIN_CODED_LEN; ++j) {
          std::cout << static_cast<char>(data[current->position + j]);
        }
        std::cout << ")" << std::endl;
#endif
        current = current->next;
        match = false;
        break;
      }
    }
    if (__builtin_expect(!match, 0)) {
      continue;
    }
    uint16_t current_match_length = match_length(data, current_pos, current);
    if (current_match_length > result.length) {
      result.length = current_match_length;
      result.position = current->position;
      result.found = true;
      if (result.length >= optimisation_threshold) {
        break;
      }
    }

    current = current->next;
  }

  // if no match was found, return the default result (found = false)
  return result;
}

uint16_t HashTable::match_length(std::vector<uint8_t>& data,
                                 uint64_t current_pos, HashNode* current) {
  uint16_t current_match_length = 0;
  for (uint16_t i = 0; i < max_additional_length; ++i) {
    auto cmp1_index = current_pos + MIN_CODED_LEN + i;
    auto cmp2_index = current->position + MIN_CODED_LEN + i;

    if (cmp1_index >= data.size() || cmp2_index >= data.size()) {
      break;
    }

    // compare characters
    if (data[cmp1_index] == data[cmp2_index]) {
      current_match_length++;
    } else {
      break;
    }
  }

  return current_match_length;
}

void HashTable::insert(std::vector<uint8_t>& data, uint64_t position) {
  HashNode* new_node = new HashNode;
  new_node->position = position;
  // hash of the MIN_CODED_LEN bytes of data at the given position
  uint32_t index = hash_function(data, position);

#if DEBUG_PRINT
  std::cout << "HashTable::insert: " << std::endl;
  std::cout << "  position: " << position << std::endl;
  std::cout << "  index: " << index << std::endl;
  std::cout << "  data: ";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<int>(data[position + i]) << " ";
  }
  std::cout << "(";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<char>(data[position + i]);
  }
  std::cout << ")" << std::endl;
  std::cout << std::endl;
#endif

  HashNode* current = table[index];
  if (current == nullptr) {
    table[index] = new_node;
    new_node->next = nullptr;
  } else {
    new_node->next = current;
    table[index] = new_node;
  }
}

void HashTable::remove(std::vector<uint8_t>& data, uint64_t position) {
  uint32_t key = hash_function(data, position);
  HashNode* current = table[key];
  HashNode* prev = nullptr;

  while (current != nullptr && current->position != position) {
    prev = current;
    current = current->next;
  }
#if DEBUG_PRINT
  std::cout << "HashTable::remove: " << std::endl;
  std::cout << "  position: " << position << std::endl;
  std::cout << "  index: " << key << std::endl;
  std::cout << "  data: ";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<int>(data[position + i]) << " ";
  }
  std::cout << "(";
  for (uint16_t i = 0; i < MIN_CODED_LEN; ++i) {
    std::cout << static_cast<char>(data[position + i]);
  }
  std::cout << ")" << std::endl;
  std::cout << std::endl;
#endif

  if (__builtin_expect(current == nullptr, 0)) {
    // this should never happen
    throw std::runtime_error("Item not found in the hash table");
    return;
  }

  // we found the node matching the data
  if (prev == nullptr) {
    // delete the head and update the head pointer
    table[key] = current->next;
  } else {
    // delete the current node and update the previous node's next pointer
    prev->next = current->next;
  }

  // free the memory of the current node
  delete current;
}

// src/hashtable.hpp
/**
 * @file      hashtable.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file for hash table implementation for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <cstdint>
#include <vector>

#include "common.hpp"

// Default size for the hash table (power of 2 for efficient masking)
#define HASH_TABLE_SIZE (1 << 12)

/**
 * @struct search_result
 * @brief Structure to hold the result of a hash table search.
 */
struct search_result {
  bool found;         // true if a match was found
  uint64_t position;  // position of the match start in the input stream
  uint16_t length;    // length of the match (beyond MIN_CODED_LEN)
};

/**
 * @class HashTable
 * @brief Implements a hash table using separate chaining to store positions of
 * byte sequences for LZSS dictionary matching.
 */
class HashTable {
  public:
  /**
   * @brief Constructs a HashTable with a specified size.
   * @param size The number of buckets in the hash table.
   */
  HashTable(uint32_t size);

  /**
   * @brief Destroys the HashTable, freeing allocated memory for nodes and the
   * table itself.
   */
  ~HashTable();

  /**
   * @brief Inserts the position of a byte sequence into the hash table.
   * Hashes the sequence starting at 'position' and adds a node to the
   * corresponding chain.
   * @param data The input data vector.
   * @param position The starting position of the sequence to insert.
   */
  void insert(std::vector<uint8_t>& data, uint64_t position);

  /**
   * @brief Removes the entry corresponding to a specific position from the hash
   * table. Used to maintain a sliding window.
   * @param data The input data vector (used for hashing).
   * @param position The starting position of the sequence to remove.
   * @throws std::runtime_error if the item to be removed is not found.
   */
  void remove(std::vector<uint8_t>& data, uint64_t position);

  /**
   * @brief Searches the hash table for the longest match for the sequence
   * starting at the current position.
   * @param data The input data vector.
   * @param current_pos The current position in the data vector to search from.
   * @return A search_result struct indicating if a match was found, its
   * position, and its length.
   */
  struct search_result search(std::vector<uint8_t>& data, uint64_t current_pos);

  private:
  // Forward declaration for the linked list node structure
  struct HashNode;

  /**
   * @struct HashNode
   * @brief Node structure for the linked list used in separate chaining.
   */
  struct HashNode {
    uint64_t position;  // Position in the input stream
    HashNode* next;     // Pointer to the next node in the chain
  };

  HashNode** table;  // Array of pointers to HashNode (the hash table buckets)
  uint32_t size;     // The size of the hash table array
  uint64_t collision_count;  // Counter for hash collisions (debug/analysis)

  private:
  /**
   * @brief Calculates the hash index for a byte sequence starting at a given
   * position.
   * @param data The input data vector.
   * @param position The starting position of the sequence to hash.
   * @return The calculated hash table index.
   */
  uint32_t hash_function(std::vector<uint8_t>& data, uint64_t position);

  /**
   * @brief Calculates the length of the match between the sequence at
   * current_pos and the sequence starting at the position stored in a HashNode.
   * Assumes the first MIN_CODED_LEN bytes already match.
   * @param data The input data vector.
   * @param current_pos The current position in the data vector.
   * @param current The HashNode pointing to the potential match position.
   * @return The length of the match beyond the initial MIN_CODED_LEN bytes.
   */
  uint16_t match_length(std::vector<uint8_t>& data, uint64_t current_pos,
                        HashNode* current);
};

#endif  // HASHTABLE_HPP

// src/image.cpp
/**
 * @file      image.cpp
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

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "block.hpp"
#include "block_reader.hpp"
#include "block_writer.hpp"
#include "common.hpp"
#include "image.hpp"
// constructor for encoding
Image::Image(std::string i_filename, std::string o_filename, uint32_t width,
             bool adaptive, bool model)
    : m_input_filename(i_filename),
      m_output_filename(o_filename),
      m_width(width),
      m_adaptive(adaptive),
      m_model(model),
      m_binary_only(true) {
  // read the input file and store it in m_data vector
  read_enc_input_file();
  if (m_data.size() != static_cast<size_t>(m_width) * m_height) {
    throw std::runtime_error(
        "Error: Data size does not match image dimensions.");
  }
}

// constructor for decoding
Image::Image(std::string i_filename, std::string o_filename)
    : m_input_filename(i_filename), m_output_filename(o_filename) {
  // read the input file and store it in m_data vector
  read_dec_input_file();
}

Image::~Image() {
  if (o_file_handle.is_open()) {
    o_file_handle.close();
  }
}

void Image::read_dec_input_file() {
  // read the input file and store the tokens in m_tokens vector
  // store all the params from header in the class variables

  read_blocks_from_file(m_input_filename, m_width, m_height, OFFSET_BITS,
                        LENGTH_BITS, m_adaptive, m_model, m_blocks,
                        m_binary_only);
}

void Image::read_enc_input_file() {
  std::ifstream file(m_input_filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Error: Unable to open input file: " +
                             m_input_filename);
  }

  file.seekg(0, std::ios::end);
  auto length = file.tellg();
  file.seekg(0, std::ios::beg);

  uint64_t temp_height = length / m_width;

  if (length > UINT32_MAX) {
    std::ostringstream error_msg;
    error_msg << "Error: Input file '" << m_input_filename
              << "' is too large. Maximum size is " << UINT16_MAX << " bytes.";
    throw std::runtime_error(error_msg.str());
  }
  m_height = static_cast<uint32_t>(temp_height);

  uint64_t expected_size = static_cast<uint64_t>(m_width) * m_height;
  if (length < 0 || static_cast<uint64_t>(length) != expected_size) {
    std::ostringstream error_msg;
    error_msg << "Error: Input file '" << m_input_filename
              << "' size mismatch. Expected " << expected_size << " bytes ("
              << m_width << "x" << m_height << "), but got " << length
              << " bytes.";
    throw std::runtime_error(error_msg.str());
  }

  m_data.resize(expected_size);
  file.read(reinterpret_cast<char*>(m_data.data()), expected_size);
  std::cout << expected_size << std::endl;
  for (auto& symbol : m_data) {
    if (symbol != 0 && symbol != 0xFF) {
      m_binary_only = false;
      break;
    }
  }
#if BINARY_ONLY
  if (m_binary_only) {
    // compress eights of bytes in m_data into one byte
    std::vector<uint8_t> compressed_data;
    compressed_data.reserve(m_data.size() / 8);
    for (size_t i = 0; i < m_data.size(); i += 8) {
      uint8_t packed_byte = 0;
      for (size_t j = 0; j < 8 && i + j < m_data.size(); ++j) {
        if (m_data[i + j] == 0xFF) {
          packed_byte |= (1 << (7 - j));
        }
      }
      compressed_data.push_back(packed_byte);
    }
    m_data.clear();
    m_data.shrink_to_fit();
    m_data = compressed_data;

    expected_size = (static_cast<uint64_t>(m_width) * m_height + 7) / 8;
    m_width = (m_width + 7) / 8;
  }
#endif

  if (m_data.size() != expected_size) {
    std::ostringstream error_msg;
    error_msg << "Error: Read incomplete data from file '" << m_input_filename
              << "'. Expected " << expected_size << " bytes, read "
              << m_data.size() << " bytes.";
    throw std::runtime_error(error_msg.str());
  }
}

void Image::write_dec_output_file() {
#if BINARY_ONLY
  if (m_binary_only) {
    std::vector<uint8_t> decompressed_data;
    bool finished_unpacking = false;

    for (uint8_t compressed_byte : m_data) {
      if (finished_unpacking)
        break;
      for (int j = 7; j >= 0; --j) {
        if ((compressed_byte >> j) & 1) {
          decompressed_data.push_back(0xFF);
        } else {
          decompressed_data.push_back(0x00);
        }
      }
    }

    m_data = decompressed_data;
  }
#endif

  // write the decoded data to the output file
  std::ofstream o_file_handle(m_output_filename, std::ios::binary);
  if (!o_file_handle) {
    throw std::runtime_error("Error: Unable to open output file: " +
                             m_output_filename);
  }
  o_file_handle.write(reinterpret_cast<const char*>(m_data.data()),
                      m_data.size());
  o_file_handle.close();
  std::cout << "Decoded data written to: " << m_output_filename << std::endl;
  std::cout << "Written " << m_data.size() << " bytes." << std::endl;
}

void Image::create_blocks() {
  if (!m_adaptive) {
    create_single_block();
  } else {
    create_multiple_blocks();
  }
}

void Image::print_blocks() {
  for (size_t i = 0; i < m_blocks.size(); i++) {
    Block& block = m_blocks[i];
    std::cout << "Block #" << i << " of width: " << block.m_width
              << " and height: " << block.m_height << std::endl;
  }
}

void Image::encode_blocks() {
  std::vector<token_t> tokens;
  // iterate over all blocks, serialize, encode and write them
  for (size_t i = 0; i < m_blocks.size(); i++) {
    Block& block = m_blocks[i];
    if (m_adaptive) {
      block.serialize_all_strategies();
      if (m_model)
        for (size_t j = 0; j < N_STRATEGIES; j++) {
#if MTF
          block.mtf(static_cast<SerializationStrategy>(j));
#else
          block.delta_transform(static_cast<SerializationStrategy>(j));
#endif
        }
      block.encode_adaptive();
    } else {
      if (m_model)
#if MTF
        block.mtf(DEFAULT);
#else
        block.delta_transform(DEFAULT);
#endif
      block.encode_using_strategy(DEFAULT);
    }
#if DEBUG_PRINT
    std::cout << "Block #" << i
              << " picked strategy: " << block.m_picked_strategy << std::endl;
#endif
#if DEBUG_COMP_ENC_UNENC
    block.decode_using_strategy(DEFAULT);
    block.compare_encoded_decoded();
#endif
#if DEBUG_PRINT_TOKENS
    block.print_tokens();
#endif
  }

  // m_data will not be needed anymore
  m_data.clear();
}

void Image::write_blocks() {
  write_blocks_to_stream(m_output_filename, m_width, m_height, OFFSET_BITS,
                         LENGTH_BITS, m_adaptive, m_model, m_blocks,
                         m_binary_only);
}

void Image::decode_blocks() {
  for (size_t i = 0; i < m_blocks.size(); i++) {
    Block& block = m_blocks[i];
    block.decode_using_strategy(DEFAULT);
#if DEBUG_PRINT_TOKENS
    block.print_tokens();
#endif
    if (m_model) {
#if MTF
      block.reverse_mtf();
#else
      block.reverse_delta_transform();
#endif
    }
    if (m_adaptive) {
      block.deserialize();
    }
  }
}

void Image::compose_image() {
  if (m_blocks.empty()) {
    std::cerr << "Warning: No blocks available to compose the image."
              << std::endl;
    m_data.clear();
    return;
  }

  uint64_t expected_size = static_cast<uint64_t>(m_width) * m_height;

  m_data.clear();

  if (expected_size != 0) {
    m_data.resize(expected_size);
  }

  if (!m_adaptive) {
    if (m_blocks.size() != 1) {
      throw std::runtime_error(
          "Error composing image: Non-adaptive mode expects exactly one "
          "block, found " +
          std::to_string(m_blocks.size()));
    }
    const Block& single_block = m_blocks[0];

    if (single_block.m_width != m_width || single_block.m_height != m_height) {
      std::cout << "Error composing image: Single block dimensions: "
                << single_block.m_width << "x" << single_block.m_height
                << ", expected: " << m_width << "x" << m_height << std::endl;
      throw std::runtime_error(
          "Error composing image: Single block dimensions mismatch image "
          "dimensions.");
    }
    if (single_block.m_decoded_data.size() != expected_size) {
      throw std::runtime_error(
          "Error composing image: Decoded data size mismatch in "
          "non-adaptive mode.");
    }

    m_data = single_block.m_decoded_data;

  } else {
    uint16_t n_blocks_rows = (m_height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint16_t n_blocks_cols = (m_width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t block_index = 0;

    for (uint16_t block_r = 0; block_r < n_blocks_rows; ++block_r) {
      uint16_t start_row = block_r * BLOCK_SIZE;

      for (uint16_t block_c = 0; block_c < n_blocks_cols; ++block_c) {
        uint16_t start_col = block_c * BLOCK_SIZE;

        if (block_index >= m_blocks.size()) {
          throw std::runtime_error(
              "Error composing image: Not enough blocks provided for image "
              "dimensions.");
        }

        Block& current_block = m_blocks[block_index];
        const std::vector<uint8_t>& block_data =
            current_block.get_decoded_data();
        uint16_t current_block_height = current_block.m_height;
        uint16_t current_block_width = current_block.m_width;

        size_t block_data_idx = 0;
        // place data into the final image using correct destination indices
        for (uint16_t r = 0; r < current_block_height; ++r) {
          for (uint16_t c = 0; c < current_block_width; ++c) {
            size_t dest_row = start_row + r;
            size_t dest_col = start_col + c;

            if (dest_row >= m_height || dest_col >= m_width) {
              throw std::runtime_error(
                  "Error composing image: Calculated destination index out "
                  "of image bounds.");
            }

            // Index into final image data
            size_t dest_index = dest_row * m_width + dest_col;
            if (dest_index >= m_data.size()) {
              throw std::runtime_error(
                  "Error: Calculated destination index out of bounds during "
                  "composition.");
            }
            if (block_data_idx >= block_data.size()) {
              throw std::runtime_error(
                  "Error: Source block data index out of bounds during "
                  "composition.");
            }

            m_data[dest_index] = block_data[block_data_idx++];
          }
        }
        block_index++;
      }
    }

    if (block_index != m_blocks.size()) {
      std::cerr << "Warning: Composed image using " << block_index
                << " blocks, but " << m_blocks.size()
                << " were decoded. File might be incomplete." << std::endl;
    }
  }

  if (m_data.size() != expected_size) {
    throw std::runtime_error(
        "Error composing image: Final composed image size (" +
        std::to_string(m_data.size()) + ") does not match expected size (" +
        std::to_string(expected_size) + ").");
  }
}

uint32_t Image::get_width() {
  return m_width;
}

uint32_t Image::get_height() {
  return m_height;
}

bool Image::is_adaptive() {
  return m_adaptive;
}

bool Image::is_compression_successful() {
  size_t coded = 0;
  size_t uncoded = 0;
  for (auto& block : m_blocks) {
    auto strategy = block.m_picked_strategy;
    coded += block.m_strategy_results[strategy].n_coded_tokens;
    uncoded += block.m_strategy_results[strategy].n_unencoded_tokens;
  }
  size_t file_header_bits =
      32 + 32 + 16 + 16 + 1 +
      1;  // width, height, offset, length, model, adaptive

  // block size
  if (m_adaptive) {
    file_header_bits += 16;
  }

  size_t total_token_bits =
      (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded);

  size_t total_strategy_bits = m_blocks.size() * 2;
  size_t total_size_bits =
      file_header_bits + total_token_bits + total_strategy_bits;

  size_t size_original = static_cast<size_t>(m_width) * m_height;

  size_t compressed_size = static_cast<size_t>(ceil(total_size_bits / 8.0));

  // std::cout << "--- Compression Stats ---" << std::endl;
  std::cout << "Original Size: " << size_original << " bytes" << std::endl;
  std::cout << "Compressed Size: " << compressed_size << " bytes" << std::endl;
  std::cout << "Space saving: "
            << 1 - static_cast<double>(compressed_size) / size_original
            << std::endl;
  return compressed_size < size_original;
}

void Image::reverse_transform() {
  for (auto& block : m_blocks) {
    block.reverse_delta_transform();
  }
}

void Image::copy_unsuccessful_compression() {
  // open the output file in binary mode
  std::ofstream o_file_handle(m_output_filename, std::ios::binary);

  // write a zero byte to the output file
  o_file_handle.put(0);
  // open the input file and copy the data to the output file
  std::ifstream i_file_handle(m_input_filename, std::ios::binary);
  if (!i_file_handle) {
    throw std::runtime_error("Error: Unable to open input file: " +
                             m_input_filename);
  }
  o_file_handle << i_file_handle.rdbuf();
  o_file_handle.close();
  i_file_handle.close();
  std::cout << "Unsuccessful compression, original data copied to: "
            << m_output_filename << std::endl;
}

void Image::create_single_block() {
  // single block
  m_blocks.reserve(1);
  m_blocks.push_back(Block(m_data, m_width, m_width));
}

void Image::create_multiple_blocks() {
  m_blocks.clear();
  uint16_t n_blocks_rows = (m_height + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint16_t n_blocks_cols = (m_width + BLOCK_SIZE - 1) / BLOCK_SIZE;
  m_blocks.reserve(static_cast<size_t>(n_blocks_rows) * n_blocks_cols);

  for (uint16_t block_r = 0; block_r < n_blocks_rows; ++block_r) {
    uint16_t start_row = block_r * BLOCK_SIZE;
    uint16_t current_block_height =
        std::min<uint16_t>(BLOCK_SIZE, m_height - start_row);

    for (uint16_t block_c = 0; block_c < n_blocks_cols; ++block_c) {
      uint16_t start_col = block_c * BLOCK_SIZE;
      uint16_t current_block_width =
          std::min<uint16_t>(BLOCK_SIZE, m_width - start_col);

      std::vector<uint8_t> block_data;
      block_data.reserve(static_cast<size_t>(current_block_width) *
                         current_block_height);

      for (uint16_t r = start_row; r < start_row + current_block_height; ++r) {
        for (uint16_t c = start_col; c < start_col + current_block_width; ++c) {
          size_t index = static_cast<size_t>(r) * m_width + c;
          if (index >= m_data.size()) {
            throw std::runtime_error(
                "Error: Calculated index out of bounds during block "
                "creation.");
          }
          block_data.push_back(m_data[index]);
        }
      }

      if (block_data.size() !=
          static_cast<size_t>(current_block_width) * current_block_height) {
        std::cerr << "Error: Internal logic error during block creation. "
                     "Size mismatch."
                  << " Expected: "
                  << (static_cast<size_t>(current_block_width) *
                      current_block_height)
                  << " Got: " << block_data.size() << std::endl;
        throw std::runtime_error("Block data size mismatch during creation.");
      }

      m_blocks.emplace_back(std::move(block_data), current_block_width,
                            current_block_height);
    }
  }

  if (m_blocks.size() != static_cast<size_t>(n_blocks_rows) * n_blocks_cols) {
    std::cerr << "Warning: Number of created blocks (" << m_blocks.size()
              << ") does not match expected count ("
              << (static_cast<size_t>(n_blocks_rows) * n_blocks_cols) << ")."
              << std::endl;
  }
}

// src/image.hpp
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

// src/lz_codec.cpp
/**
 * @file      lz_codec.c
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     LZSS Compressor and Decompressor main file
 *
 * @date      12 April  2025 \n
 */

#include <assert.h>
#include <math.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "argparser.hpp"
#include "image.hpp"

uint16_t BLOCK_SIZE = DEFAULT_BLOCK_SIZE;

// coded token parameters
uint16_t OFFSET_BITS = DEFAULT_OFFSET_BITS;
uint16_t LENGTH_BITS = DEFAULT_LENGTH_BITS;

// max number expressible with the OFFSET_LENGTH bits
uint16_t SEARCH_BUF_SIZE = (1 << OFFSET_BITS) - 1;

// designates the max length in longest prefix searching
// finds the maximum value we can represent with the given number of bits
// and add the minimum coded length to optimize for value mapping
uint16_t MAX_CODED_LEN = (1 << LENGTH_BITS) - 1 + MIN_CODED_LEN;

// used only for statistics printing
size_t TOKEN_CODED_LEN = 1 + OFFSET_BITS + LENGTH_BITS;
size_t TOKEN_UNCODED_LEN = 1 + 8;

void print_final_stats(Image& img) {
  size_t coded = 0;
  size_t uncoded = 0;
  for (auto& block : img.m_blocks) {
    auto strategy = block.m_picked_strategy;
    for (auto& token : block.m_tokens[strategy]) {
      token.coded ? coded++ : uncoded++;
    }
  }
  size_t file_header_bits =
      32 + 32 + 16 + 16 + 1 +
      1;  // width, height, offset, length, model, adaptive

  // block size
  if (img.is_adaptive()) {
    file_header_bits += 16;
  }

  size_t total_token_bits =
      (TOKEN_CODED_LEN * coded) + (TOKEN_UNCODED_LEN * uncoded);

  size_t total_strategy_bits = img.m_blocks.size() * 2;
  size_t total_size_bits =
      file_header_bits + total_token_bits + total_strategy_bits;

  size_t size_original =
      static_cast<size_t>(img.get_width()) * img.get_height() * 8;
  double compression_ratio =
      (total_size_bits > 0)
          ? (static_cast<double>(size_original) / total_size_bits)
          : 0.0;
  size_t total_size_bytes = static_cast<size_t>(ceil(total_size_bits / 8.0));

  std::cout << "--- Final Stats ---" << std::endl;
  std::cout << "Image Dimensions: " << img.get_width() << "x"
            << img.get_height() << std::endl;
  std::cout << "Adaptive Mode: " << (img.is_adaptive() ? "Yes" : "No")
            << std::endl;
  if (img.is_adaptive()) {
    std::cout << "Block Size: " << BLOCK_SIZE << "x" << BLOCK_SIZE << std::endl;
  }
  std::cout << "Number of Blocks: " << img.m_blocks.size() << std::endl;
  std::cout << "Offset Bits: " << OFFSET_BITS
            << ", Length Bits: " << LENGTH_BITS << std::endl;
  std::cout << "Original data size: " << size_original << "b ("
            << size_original / 8 << "B)" << std::endl;
  std::cout << "Coded tokens: " << coded << " (" << TOKEN_CODED_LEN * coded
            << "b)" << std::endl;
  std::cout << "Uncoded tokens: " << uncoded << " ("
            << TOKEN_UNCODED_LEN * uncoded << "b)" << std::endl;
  std::cout << "File Header Size: " << file_header_bits << "b" << std::endl;
  std::cout << "Block Strategy Headers Size: " << total_strategy_bits << "b"
            << std::endl;
  std::cout << "Total Token Data Size: " << total_token_bits << "b"
            << std::endl;
  std::cout << "Calculated Total Size: " << total_size_bits << "b ("
            << total_size_bytes << "B)" << std::endl;
  std::cout << "Compression Ratio (Original Bits / Total Bits): "
            << compression_ratio << std::endl;
  double space_saved_percent =
      (size_original > 0)
          ? (1.0 - (static_cast<double>(total_size_bits) / size_original)) *
                100.0
          : 0.0;

  std::cout << "Space Saved: " << std::fixed << std::setprecision(2)
            << space_saved_percent << "%" << std::endl;
}

bool copy_uncompressed_file(std::string input_filename,
                            std::string output_filename) {
  // open the input file in binary mode
  std::ifstream i_file_handle(input_filename, std::ios::binary);

  // read the first byte from the input file
  char first_byte;
  i_file_handle.get(first_byte);

  if (first_byte) {
    i_file_handle.close();

    return false;
  }

  // open the output file
  std::ifstream o_file_handle(output_filename, std::ios::binary);
  // copy the contents of input file into the output file
  std::ofstream o_file_handle_copy(output_filename, std::ios::binary);
  if (!o_file_handle_copy) {
    throw std::runtime_error("Error: Unable to open output file: " +
                             output_filename);
  }
  o_file_handle_copy << i_file_handle.rdbuf();
  o_file_handle_copy.close();
  i_file_handle.close();
  return true;
}

int main(int argc, char* argv[]) {
  ArgumentParser args(argc, argv);

  SEARCH_BUF_SIZE = (1 << OFFSET_BITS) - 1;
  MAX_CODED_LEN = (1 << LENGTH_BITS) - 1 + MIN_CODED_LEN;
  TOKEN_CODED_LEN = 1 + OFFSET_BITS + LENGTH_BITS;
  TOKEN_UNCODED_LEN = 1 + 8;

  // asserts for checking valid values
  assert(BLOCK_SIZE > 0 && BLOCK_SIZE < (1 << 15));
  assert(OFFSET_BITS > 0 && OFFSET_BITS < 16);
  assert(LENGTH_BITS > 0 && LENGTH_BITS < 16);
  assert(MIN_CODED_LEN > 0);

  if (args.is_compress_mode()) {
    Image i =
        Image(args.get_input_file(), args.get_output_file(),
              args.get_image_width(), args.is_adaptive(), args.use_model());
    i.create_blocks();
    i.encode_blocks();
    if (i.is_compression_successful()) {
      i.write_blocks();
    } else {
      i.copy_unsuccessful_compression();
      // print_final_stats(i);
    }
  } else {
    if (copy_uncompressed_file(args.get_input_file(), args.get_output_file())) {
      return 0;
    }
    Image i = Image(args.get_input_file(), args.get_output_file());
    i.decode_blocks();
    i.compose_image();
    i.write_dec_output_file();
  }

  return 0;
}

// src/token.hpp
/**
 * @file      token.hpp
 *
 * @author    Pavel Kratochvil \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            xkrato61@fit.vutbr.cz
 *
 * @brief     Header file with token structure for LZSS compression
 *
 * @date      12 April  2025 \n
 */

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

/**
 * @struct token_t
 * @brief Represents a token in the LZSS compressed stream.
 *
 * A token can either represent a literal (uncoded) byte or a reference
 * (coded) to a previously seen sequence of bytes, defined by an offset
 * and length pair.
 */
typedef struct {
  bool coded;
  union {
    uint8_t value;
    struct {
      uint16_t offset;
      uint16_t length;
    };
  } data;

} token_t;

#endif  // TOKEN_HPP
