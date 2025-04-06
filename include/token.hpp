#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>

struct token_t {
  bool coded;         // true if the token is coded
  uint16_t length;    // length of the token
  uint16_t position;  // position of the token in the input stream
};

#endif  // TOKEN_HPP