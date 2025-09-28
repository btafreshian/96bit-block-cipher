#include <array>
#include <iomanip>
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

constexpr std::array<std::array<std::uint8_t, cube96::kBlockBytes>, cube96::kRoundCount>
    kExpectedRoundKeys = {{{0x5E, 0xEA, 0x71, 0x1B, 0x1A, 0x0E, 0xC8, 0x95, 0x36, 0x85, 0x23, 0x4E},
                           {0xDD, 0xAA, 0x77, 0x93, 0xFB, 0x42, 0x06, 0x7D, 0xF0, 0xE4, 0xDB, 0xD0},
                           {0xED, 0x96, 0x2A, 0x80, 0xEB, 0xBC, 0x16, 0xFF, 0xDB, 0x12, 0xAF, 0x12},
                           {0xFE, 0x43, 0x48, 0xD3, 0xC8, 0x48, 0x41, 0xB6, 0xA3, 0xFD, 0x1D, 0x29},
                           {0xE7, 0xC6, 0xB3, 0xBF, 0x61, 0x66, 0xDC, 0x86, 0x87, 0x30, 0xA8, 0x49},
                           {0x49, 0xF1, 0x44, 0x0F, 0x65, 0xD3, 0x98, 0x3E, 0x46, 0x69, 0x3C, 0xEF},
                           {0xDB, 0x4C, 0xD5, 0x8E, 0x5B, 0xC6, 0x64, 0xC5, 0xB9, 0xD2, 0xC0, 0xAA},
                           {0x7C, 0xE6, 0xE4, 0x4D, 0x10, 0x89, 0x63, 0x99, 0xE3, 0xF4, 0x36, 0x6E}}};

constexpr std::array<std::array<std::uint8_t, 8>, cube96::kRoundCount> kExpectedPermSeeds = {
    {{0xF1, 0xCA, 0x09, 0xAC, 0x90, 0x42, 0xF7, 0x72},
     {0x41, 0xCA, 0xB0, 0xB7, 0xF9, 0x5A, 0x09, 0xBC},
     {0xAA, 0x56, 0x71, 0x3E, 0x55, 0x47, 0x7C, 0x3E},
     {0x6F, 0x14, 0x38, 0x5D, 0xDF, 0x47, 0x9B, 0x42},
     {0xBA, 0xCF, 0x1F, 0xCD, 0x7C, 0x9D, 0x78, 0x50},
     {0xC2, 0x60, 0x6E, 0x6D, 0xE2, 0xD7, 0xAC, 0xCE},
     {0x3D, 0xAE, 0x88, 0x50, 0x7A, 0xF5, 0x76, 0x79},
     {0x19, 0x35, 0x65, 0x36, 0xF4, 0xE0, 0x45, 0x3F}}};

constexpr cube96::RoundKey kExpectedPost = {0x88, 0x89, 0x8D, 0x0E, 0xA5, 0x24,
                                            0xC7, 0xF2, 0x7D, 0xE1, 0xE5, 0xAE};

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

  if (material1.round_keys != kExpectedRoundKeys) {
    std::cerr << "HKDF round key mismatch\n";
    for (std::size_t r = 0; r < cube96::kRoundCount; ++r) {
      std::cerr << "  rk[" << r << "] expected=";
      for (auto b : kExpectedRoundKeys[r]) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b);
      }
      std::cerr << " actual=";
      for (auto b : material1.round_keys[r]) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b);
      }
      std::cerr << std::dec << "\n";
    }
    return 1;
  }
  if (material1.perm_seeds != kExpectedPermSeeds) {
    std::cerr << "HKDF permutation seed mismatch\n";
    for (std::size_t r = 0; r < cube96::kRoundCount; ++r) {
      std::cerr << "  ps[" << r << "] expected=";
      for (auto b : kExpectedPermSeeds[r]) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b);
      }
      std::cerr << " actual=";
      for (auto b : material1.perm_seeds[r]) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b);
      }
      std::cerr << std::dec << "\n";
    }
    return 1;
  }
  if (material1.post_whitening != kExpectedPost) {
    std::cerr << "HKDF post-whitening mismatch\n";
    std::cerr << "  expected=";
    for (auto b : kExpectedPost) {
      std::cerr << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(b);
    }
    std::cerr << " actual=";
    for (auto b : material1.post_whitening) {
      std::cerr << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(b);
    }
    std::cerr << std::dec << "\n";
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
