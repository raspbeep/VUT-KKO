#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>

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