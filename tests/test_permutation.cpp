#include <array>
#include <iostream>

#include "cube96/endian.hpp"
#include "cube96/key_schedule.hpp"
#include "cube96/perm.hpp"
#include "cube96/types.hpp"

int main() {
  std::array<std::uint8_t, cube96::kKeyBytes> key{};
  for (std::size_t i = 0; i < key.size(); ++i) {
    key[i] = static_cast<std::uint8_t>(i * 3u + 1u);
  }

  auto material = cube96::derive_material(key.data());
  const auto &prims = cube96::primitive_set();

  for (std::size_t r = 0; r < cube96::kRoundCount; ++r) {
    std::uint64_t seed = cube96::load_be64(material.perm_seeds[r].data());
    cube96::SplitMix64 rng(seed);
    cube96::Permutation perm = cube96::identity_permutation();
    for (int step = 0; step < 12; ++step) {
      std::uint32_t pick = static_cast<std::uint32_t>(rng.next() % prims.size());
      perm = cube96::compose(perm, prims[pick]);
    }
    cube96::Permutation inv = cube96::invert(perm);

    std::array<bool, cube96::kPermSize> seen{};
    for (std::size_t src = 0; src < cube96::kPermSize; ++src) {
      std::uint8_t dst = perm[src];
      if (dst >= cube96::kPermSize) {
        std::cerr << "Permutation entry out of range\n";
        return 1;
      }
      if (seen[dst]) {
        std::cerr << "Permutation not bijective at round " << r << "\n";
        return 1;
      }
      seen[dst] = true;
      if (inv[dst] != src) {
        std::cerr << "Inverse mismatch at round " << r << "\n";
        return 1;
      }
    }
  }

  std::cout << "test_permutation: OK\n";
  return 0;
}
