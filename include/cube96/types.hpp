#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cube96 {

constexpr std::size_t kBlockBytes = 12;   // 96 bits
constexpr std::size_t kKeyBytes   = 12;   // 96 bits
constexpr std::size_t kRoundCount = 8;
constexpr std::size_t kPermSize   = 96;   // bits per block

using Block      = std::array<std::uint8_t, kBlockBytes>;
using RoundKey   = std::array<std::uint8_t, kBlockBytes>;
using Permutation = std::array<std::uint8_t, kPermSize>;

#if defined(CUBE96_LAYOUT_ROWMAJOR)

// Row-major layout: bits are ordered primarily by y, then by z, then by x.
// This maps each logical row (4×6 bits = 24 bits) to three consecutive bytes.
constexpr std::uint8_t idx_of(std::uint8_t x, std::uint8_t y, std::uint8_t z) {
  return static_cast<std::uint8_t>(((y * 6u) + z) * 4u + x);
}

inline void xyz_of(std::uint8_t idx, std::uint8_t &x, std::uint8_t &y,
                   std::uint8_t &z) {
  std::uint8_t yz = static_cast<std::uint8_t>(idx / 4u);
  x = static_cast<std::uint8_t>(idx % 4u);
  y = static_cast<std::uint8_t>(yz / 6u);
  z = static_cast<std::uint8_t>(yz % 6u);
}

inline std::uint8_t byte_index_of_bit(std::uint8_t bit_index) {
  return static_cast<std::uint8_t>(bit_index / 8u);
}

inline std::uint8_t bit_offset_in_byte(std::uint8_t bit_index) {
  return static_cast<std::uint8_t>(7u - (bit_index % 8u));
}

#else

// Z-slice layout (default): each z-slice stores 16 bits ordered by rows (y)
// and columns (x) with MSB-first packing inside each byte. Every slice spans
// two consecutive bytes in memory.
constexpr std::uint8_t idx_of(std::uint8_t x, std::uint8_t y, std::uint8_t z) {
  return static_cast<std::uint8_t>(16u * z + 4u * y + x);
}

inline void xyz_of(std::uint8_t idx, std::uint8_t &x, std::uint8_t &y,
                   std::uint8_t &z) {
  z = static_cast<std::uint8_t>(idx / 16u);
  std::uint8_t in_slice = static_cast<std::uint8_t>(idx % 16u);
  y = static_cast<std::uint8_t>(in_slice / 4u);
  x = static_cast<std::uint8_t>(in_slice % 4u);
}

inline std::uint8_t byte_index_of_bit(std::uint8_t bit_index) {
  std::uint8_t z = static_cast<std::uint8_t>(bit_index / 16u);
  std::uint8_t offset = static_cast<std::uint8_t>(bit_index % 16u);
  std::uint8_t byte_in_slice = static_cast<std::uint8_t>(offset / 8u);
  return static_cast<std::uint8_t>(2u * z + byte_in_slice);
}

inline std::uint8_t bit_offset_in_byte(std::uint8_t bit_index) {
  std::uint8_t offset = static_cast<std::uint8_t>(bit_index % 16u);
  std::uint8_t bit_in_byte = static_cast<std::uint8_t>(offset % 8u);
  return static_cast<std::uint8_t>(7u - bit_in_byte);
}

#endif

inline std::uint8_t get_bit(const std::uint8_t s[kBlockBytes],
                            std::uint8_t bit_index) {
  std::uint8_t byte = byte_index_of_bit(bit_index);
  std::uint8_t bit_pos = bit_offset_in_byte(bit_index);
  return static_cast<std::uint8_t>((s[byte] >> bit_pos) & 0x01u);
}

inline void set_bit(std::uint8_t s[kBlockBytes], std::uint8_t bit_index,
                    std::uint8_t bit) {
  std::uint8_t byte = byte_index_of_bit(bit_index);
  std::uint8_t bit_pos = bit_offset_in_byte(bit_index);
  std::uint8_t mask = static_cast<std::uint8_t>(1u << bit_pos);
  if (bit) {
    s[byte] = static_cast<std::uint8_t>(s[byte] | mask);
  } else {
    s[byte] = static_cast<std::uint8_t>(s[byte] & static_cast<std::uint8_t>(~mask));
  }
}

} // namespace cube96
