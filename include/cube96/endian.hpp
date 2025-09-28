#pragma once

#include <cstdint>
#include <cstddef>

namespace cube96 {

inline std::uint32_t load_be32(const std::uint8_t b[4]) {
  return (static_cast<std::uint32_t>(b[0]) << 24) |
         (static_cast<std::uint32_t>(b[1]) << 16) |
         (static_cast<std::uint32_t>(b[2]) << 8) |
         static_cast<std::uint32_t>(b[3]);
}

inline std::uint64_t load_be64(const std::uint8_t b[8]) {
  return (static_cast<std::uint64_t>(b[0]) << 56) |
         (static_cast<std::uint64_t>(b[1]) << 48) |
         (static_cast<std::uint64_t>(b[2]) << 40) |
         (static_cast<std::uint64_t>(b[3]) << 32) |
         (static_cast<std::uint64_t>(b[4]) << 24) |
         (static_cast<std::uint64_t>(b[5]) << 16) |
         (static_cast<std::uint64_t>(b[6]) << 8) |
         static_cast<std::uint64_t>(b[7]);
}

inline void store_be32(std::uint32_t v, std::uint8_t out[4]) {
  out[0] = static_cast<std::uint8_t>((v >> 24) & 0xFFu);
  out[1] = static_cast<std::uint8_t>((v >> 16) & 0xFFu);
  out[2] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
  out[3] = static_cast<std::uint8_t>(v & 0xFFu);
}

inline void store_be64(std::uint64_t v, std::uint8_t out[8]) {
  out[0] = static_cast<std::uint8_t>((v >> 56) & 0xFFu);
  out[1] = static_cast<std::uint8_t>((v >> 48) & 0xFFu);
  out[2] = static_cast<std::uint8_t>((v >> 40) & 0xFFu);
  out[3] = static_cast<std::uint8_t>((v >> 32) & 0xFFu);
  out[4] = static_cast<std::uint8_t>((v >> 24) & 0xFFu);
  out[5] = static_cast<std::uint8_t>((v >> 16) & 0xFFu);
  out[6] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
  out[7] = static_cast<std::uint8_t>(v & 0xFFu);
}

} // namespace cube96
