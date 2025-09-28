#include <array>
#include <cmath>
#include <iostream>

#include "cube96/cipher.hpp"
#include "cube96/types.hpp"

namespace {

std::size_t hamming_distance(const std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> &a,
                             const std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> &b) {
  std::size_t bits = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    std::uint8_t diff = static_cast<std::uint8_t>(a[i] ^ b[i]);
    diff = static_cast<std::uint8_t>((diff & 0x55u) + ((diff >> 1) & 0x55u));
    diff = static_cast<std::uint8_t>((diff & 0x33u) + ((diff >> 2) & 0x33u));
    diff = static_cast<std::uint8_t>((diff & 0x0Fu) + ((diff >> 4) & 0x0Fu));
    bits += diff;
  }
  return bits;
}

void flip_bit(std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> &data,
              std::uint8_t bit_index) {
  std::uint8_t byte = cube96::byte_index_of_bit(bit_index);
  std::uint8_t bit = cube96::bit_offset_in_byte(bit_index);
  data[byte] ^= static_cast<std::uint8_t>(1u << bit);
}

} // namespace

int main() {
  cube96::CubeCipher cipher(cube96::CubeCipher::Impl::Fast);
  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  for (std::size_t i = 0; i < key.size(); ++i) {
    key[i] = static_cast<std::uint8_t>(0xAA ^ static_cast<std::uint8_t>(i * 7u));
  }
  cipher.setKey(key.data());

  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> plain{};
  for (std::size_t i = 0; i < plain.size(); ++i) {
    plain[i] = static_cast<std::uint8_t>(i * 9u);
  }

  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> base_cipher{};
  cipher.encryptBlock(plain.data(), base_cipher.data());

  double total_plain_flip = 0.0;
  for (std::uint8_t bit = 0; bit < cube96::kPermSize; ++bit) {
    auto mutated = plain;
    flip_bit(mutated, bit);
    std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> out{};
    cipher.encryptBlock(mutated.data(), out.data());
    total_plain_flip += static_cast<double>(hamming_distance(base_cipher, out));
  }
  double avg_plain = total_plain_flip / static_cast<double>(cube96::kPermSize);
  if (avg_plain < 40.0 || avg_plain > 56.0) {
    std::cerr << "Plaintext avalanche average " << avg_plain << " out of range\n";
    return 1;
  }

  double total_key_flip = 0.0;
  for (std::uint8_t bit = 0; bit < cube96::kPermSize; ++bit) {
    auto mutated_key = key;
    flip_bit(mutated_key, bit);
    cipher.setKey(mutated_key.data());
    std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> out{};
    cipher.encryptBlock(plain.data(), out.data());
    total_key_flip += static_cast<double>(hamming_distance(base_cipher, out));
  }
  double avg_key = total_key_flip / static_cast<double>(cube96::kPermSize);
  if (avg_key < 40.0 || avg_key > 56.0) {
    std::cerr << "Key avalanche average " << avg_key << " out of range\n";
    return 1;
  }

  std::cout << "test_avalanche: OK\n";
  return 0;
}
