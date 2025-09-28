#include <array>
#include <iostream>

#include "cube96/key_schedule.hpp"

namespace {

template <typename ContainerA, typename ContainerB>
std::size_t hamming_bytes(const ContainerA &a, const ContainerB &b) {
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

} // namespace

int main() {
  std::array<std::uint8_t, cube96::kKeyBytes> key{};
  for (std::size_t i = 0; i < key.size(); ++i) {
    key[i] = static_cast<std::uint8_t>(i);
  }
  auto material1 = cube96::derive_material(key.data());
  auto material2 = cube96::derive_material(key.data());

  if (material1.round_keys != material2.round_keys ||
      material1.perm_seeds != material2.perm_seeds ||
      material1.post_whitening != material2.post_whitening) {
    std::cerr << "HKDF determinism failure\n";
    return 1;
  }

  key[0] ^= 0x80u;
  auto material3 = cube96::derive_material(key.data());

  std::size_t diff_bits = 0;
  for (std::size_t r = 0; r < cube96::kRoundCount; ++r) {
    diff_bits += hamming_bytes(material1.round_keys[r], material3.round_keys[r]);
    diff_bits += hamming_bytes(material1.perm_seeds[r], material3.perm_seeds[r]);
  }
  diff_bits += hamming_bytes(material1.post_whitening, material3.post_whitening);

  if (diff_bits == 0) {
    std::cerr << "HKDF avalanche failure\n";
    return 1;
  }

  std::cout << "test_kdf: OK\n";
  return 0;
}
