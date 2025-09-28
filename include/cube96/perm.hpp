#pragma once

#include <array>
#include <cstdint>

#include "cube96/types.hpp"

namespace cube96 {

struct SplitMix64 {
  std::uint64_t s;
  explicit SplitMix64(std::uint64_t seed) : s(seed) {}
  std::uint64_t next();
};

Permutation identity_permutation();
Permutation compose(const Permutation &accum, const Permutation &step);
Permutation invert(const Permutation &p);

void apply_permutation(const Permutation &p, const std::uint8_t in[kBlockBytes],
                       std::uint8_t out[kBlockBytes]);
void apply_permutation_ct(const Permutation &p, const std::uint8_t in[kBlockBytes],
                          std::uint8_t out[kBlockBytes]);

const std::array<Permutation, 36> &primitive_set();

} // namespace cube96
