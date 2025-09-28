#pragma once

#include <cstdint>

namespace cube96 {

inline std::uint8_t ct_eq(std::uint8_t a, std::uint8_t b) {
  std::uint8_t x = static_cast<std::uint8_t>(a ^ b);
  x |= static_cast<std::uint8_t>(x >> 4);
  x |= static_cast<std::uint8_t>(x >> 2);
  x |= static_cast<std::uint8_t>(x >> 1);
  return static_cast<std::uint8_t>(~x & 1u);
}

inline std::uint8_t ct_is_nonzero(std::uint8_t a) {
  std::uint8_t x = static_cast<std::uint8_t>(a | static_cast<std::uint8_t>(a >> 4));
  x |= static_cast<std::uint8_t>(x >> 2);
  x |= static_cast<std::uint8_t>(x >> 1);
  return static_cast<std::uint8_t>(x & 1u);
}

inline std::uint8_t ct_mask(std::uint8_t bit) {
  return static_cast<std::uint8_t>(0u - static_cast<std::uint8_t>(bit & 1u));
}

inline std::uint8_t ct_select(std::uint8_t mask, std::uint8_t a,
                              std::uint8_t b) {
  return static_cast<std::uint8_t>((mask & a) | (~mask & b));
}

inline void ct_write_bit(std::uint8_t s[], std::uint8_t byte_index,
                         std::uint8_t bit_pos, std::uint8_t bit) {
  std::uint8_t mask = static_cast<std::uint8_t>(1u << bit_pos);
  std::uint8_t neg = ct_mask(bit);
  s[byte_index] = static_cast<std::uint8_t>((s[byte_index] & static_cast<std::uint8_t>(~mask)) |
                                            (neg & mask));
}

} // namespace cube96
