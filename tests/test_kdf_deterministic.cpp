#include <array>
#include <cstdint>
#include <iostream>

#include "cube96/key_schedule.hpp"

int main() {
  std::array<std::uint8_t, cube96::kKeyBytes> key{};
  for (std::size_t i = 0; i < key.size(); ++i) {
    key[i] = static_cast<std::uint8_t>(i);
  }

  auto material = cube96::derive_material(key.data());

  const std::array<cube96::RoundKey, cube96::kRoundCount> expected_rounds = {{{
      0x5e, 0xea, 0x71, 0x1b, 0x1a, 0x0e, 0xc8, 0x95, 0x36, 0x85, 0x23, 0x4e,
  },
                                                                             {
                                                                                 0xdd, 0xaa, 0x77, 0x93, 0xfb, 0x42,
                                                                                 0x06, 0x7d, 0xf0, 0xe4, 0xdb, 0xd0,
                                                                             },
                                                                             {
                                                                                 0xed, 0x96, 0x2a, 0x80, 0xeb, 0xbc,
                                                                                 0x16, 0xff, 0xdb, 0x12, 0xaf, 0x12,
                                                                             },
                                                                             {
                                                                                 0xfe, 0x43, 0x48, 0xd3, 0xc8, 0x48,
                                                                                 0x41, 0xb6, 0xa3, 0xfd, 0x1d, 0x29,
                                                                             },
                                                                             {
                                                                                 0xe7, 0xc6, 0xb3, 0xbf, 0x61, 0x66,
                                                                                 0xdc, 0x86, 0x87, 0x30, 0xa8, 0x49,
                                                                             },
                                                                             {
                                                                                 0x49, 0xf1, 0x44, 0x0f, 0x65, 0xd3,
                                                                                 0x98, 0x3e, 0x46, 0x69, 0x3c, 0xef,
                                                                             },
                                                                             {
                                                                                 0xdb, 0x4c, 0xd5, 0x8e, 0x5b, 0xc6,
                                                                                 0x64, 0xc5, 0xb9, 0xd2, 0xc0, 0xaa,
                                                                             },
                                                                             {
                                                                                 0x7c, 0xe6, 0xe4, 0x4d, 0x10, 0x89,
                                                                                 0x63, 0x99, 0xe3, 0xf4, 0x36, 0x6e,
                                                                             }}};

  const std::array<std::array<std::uint8_t, 8>, cube96::kRoundCount> expected_seeds = {{{
      0xf1, 0xca, 0x09, 0xac, 0x90, 0x42, 0xf7, 0x72,
  },
                                                                                       {
                                                                                           0x41, 0xca, 0xb0, 0xb7, 0xf9, 0x5a,
                                                                                           0x09, 0xbc,
                                                                                       },
                                                                                       {
                                                                                           0xaa, 0x56, 0x71, 0x3e, 0x55, 0x47,
                                                                                           0x7c, 0x3e,
                                                                                       },
                                                                                       {
                                                                                           0x6f, 0x14, 0x38, 0x5d, 0xdf, 0x47,
                                                                                           0x9b, 0x42,
                                                                                       },
                                                                                       {
                                                                                           0xba, 0xcf, 0x1f, 0xcd, 0x7c, 0x9d,
                                                                                           0x78, 0x50,
                                                                                       },
                                                                                       {
                                                                                           0xc2, 0x60, 0x6e, 0x6d, 0xe2, 0xd7,
                                                                                           0xac, 0xce,
                                                                                       },
                                                                                       {
                                                                                           0x3d, 0xae, 0x88, 0x50, 0x7a, 0xf5,
                                                                                           0x76, 0x79,
                                                                                       },
                                                                                       {
                                                                                           0x19, 0x35, 0x65, 0x36, 0xf4, 0xe0,
                                                                                           0x45, 0x3f,
                                                                                       }}};

  const cube96::RoundKey expected_post = {0x88, 0x89, 0x8d, 0x0e, 0xa5, 0x24,
                                          0xc7, 0xf2, 0x7d, 0xe1, 0xe5, 0xae};

  bool ok = true;
  if (material.round_keys != expected_rounds) {
    std::cerr << "Round key mismatch\n";
    ok = false;
  }
  if (material.perm_seeds != expected_seeds) {
    std::cerr << "Permutation seed mismatch\n";
    ok = false;
  }
  if (material.post_whitening != expected_post) {
    std::cerr << "Post-whitening key mismatch\n";
    ok = false;
  }

  if (!ok) {
    return 1;
  }
  std::cout << "test_kdf_deterministic: OK\n";
  return 0;
}
