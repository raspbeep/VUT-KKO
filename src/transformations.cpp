#include "transformations.hpp"

void rle(std::vector<uint8_t>& data) {
  if (data.empty()) {
    return;
  }

  std::vector<uint8_t> encoded_data;
  encoded_data.reserve(data.size());
  size_t i = 0;

  while (i < data.size()) {
    uint8_t current = data[i];
    size_t count = 1;
    while (i + count < data.size() && data[i + count] == current &&
           count < 255 + 3) {
      count++;
    }

    if (count < 3) {
      for (size_t j = 0; j < count; j++) {
        encoded_data.push_back(current);
      }
    } else {
      encoded_data.push_back(current);
      encoded_data.push_back(current);
      encoded_data.push_back(current);
      encoded_data.push_back(static_cast<uint8_t>(count - 3));
    }
    i += count;
  }

  data.swap(encoded_data);
}

void reverse_rle(std::vector<uint8_t>& data) {
  if (data.empty()) {
    return;
  }

  std::vector<uint8_t> decoded_data;
  decoded_data.reserve(data.size() * 2);  // Conservative estimate
  size_t i = 0;

  while (i < data.size()) {
    if (i + 3 < data.size() && data[i] == data[i + 1] &&
        data[i + 1] == data[i + 2]) {
      uint8_t value = data[i];
      uint8_t count = data[i + 3];
      for (size_t j = 0; j < static_cast<size_t>(3) + count; j++) {
        decoded_data.push_back(value);
      }
      i += 4;
    } else {
      decoded_data.push_back(data[i]);
      i++;
    }
  }

  data.swap(decoded_data);
}