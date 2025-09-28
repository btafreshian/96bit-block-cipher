#include <array>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include "cube96/endian.hpp"
#include "cube96/key_schedule.hpp"
#include "cube96/perm.hpp"
#include "cube96/sbox.hpp"
#include "cube96/types.hpp"

namespace {

bool parse_hex_block(const std::string &hex,
                     std::array<std::uint8_t, cube96::kBlockBytes> &out) {
  if (hex.size() != cube96::kBlockBytes * 2) {
    return false;
  }
  auto hex_to_int = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  for (std::size_t i = 0; i < cube96::kBlockBytes; ++i) {
    int hi = hex_to_int(hex[2 * i]);
    int lo = hex_to_int(hex[2 * i + 1]);
    if (hi < 0 || lo < 0) {
      return false;
    }
    out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return true;
}

int parity8(std::uint8_t x) {
  x ^= static_cast<std::uint8_t>(x >> 4);
  x ^= static_cast<std::uint8_t>(x >> 2);
  x ^= static_cast<std::uint8_t>(x >> 1);
  return x & 1;
}

int parity_mask(const std::array<std::uint8_t, cube96::kBlockBytes> &value,
                const std::array<std::uint8_t, cube96::kBlockBytes> &mask) {
  int parity = 0;
  for (std::size_t i = 0; i < cube96::kBlockBytes; ++i) {
    parity ^= parity8(static_cast<std::uint8_t>(value[i] & mask[i]));
  }
  return parity & 1;
}

std::array<std::uint8_t, cube96::kBlockBytes>
partial_encrypt(const std::array<std::uint8_t, cube96::kBlockBytes> &input,
                int rounds, const cube96::DerivedMaterial &material,
                const std::array<cube96::Permutation, cube96::kRoundCount> &perms) {
  std::array<std::uint8_t, cube96::kBlockBytes> state = input;
  for (int r = 0; r < rounds; ++r) {
    for (std::size_t i = 0; i < cube96::kBlockBytes; ++i) {
      state[i] = static_cast<std::uint8_t>(state[i] ^ material.round_keys[r][i]);
    }
    for (std::size_t i = 0; i < cube96::kBlockBytes; ++i) {
      state[i] = cube96::AES_SBOX[state[i]];
    }
    std::array<std::uint8_t, cube96::kBlockBytes> tmp{};
    cube96::apply_permutation(perms[r], state.data(), tmp.data());
    state = tmp;
  }
  return state;
}

} // namespace

int main(int argc, char **argv) {
  int rounds = 4;
  std::size_t samples = 1u << 16;
  std::array<std::uint8_t, cube96::kBlockBytes> key{};
  std::array<std::uint8_t, cube96::kBlockBytes> mask_in{};
  std::array<std::uint8_t, cube96::kBlockBytes> mask_out{};
  mask_in.fill(0);
  mask_out.fill(0);
  mask_in[0] = 0x01;
  mask_out[0] = 0x01;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--rounds" && i + 1 < argc) {
      rounds = std::stoi(argv[++i]);
    } else if (arg == "--samples" && i + 1 < argc) {
      samples = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--mask-in" && i + 1 < argc) {
      if (!parse_hex_block(argv[++i], mask_in)) {
        std::cerr << "Invalid input mask\n";
        return 1;
      }
    } else if (arg == "--mask-out" && i + 1 < argc) {
      if (!parse_hex_block(argv[++i], mask_out)) {
        std::cerr << "Invalid output mask\n";
        return 1;
      }
    } else if (arg == "--key" && i + 1 < argc) {
      if (!parse_hex_block(argv[++i], key)) {
        std::cerr << "Invalid key\n";
        return 1;
      }
    } else if (arg == "--help") {
      std::cout << "Usage: cube96_linear_bias [--rounds N] [--samples N]"
                   " [--mask-in HEX] [--mask-out HEX] [--key HEX]\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return 1;
    }
  }

  if (rounds < 1 || rounds > 4) {
    std::cerr << "Rounds must be between 1 and 4\n";
    return 1;
  }

  if (std::all_of(mask_in.begin(), mask_in.end(), [](std::uint8_t b) { return b == 0; }) ||
      std::all_of(mask_out.begin(), mask_out.end(), [](std::uint8_t b) { return b == 0; })) {
    std::cerr << "Masks must not be all-zero\n";
    return 1;
  }

  auto material = cube96::derive_material(key.data());
  const auto &prims = cube96::primitive_set();
  std::array<cube96::Permutation, cube96::kRoundCount> perms;
  for (std::size_t r = 0; r < cube96::kRoundCount; ++r) {
    std::uint64_t seed = cube96::load_be64(material.perm_seeds[r].data());
    cube96::SplitMix64 prng(seed);
    cube96::Permutation perm = cube96::identity_permutation();
    for (int step = 0; step < 12; ++step) {
      std::uint32_t pick = static_cast<std::uint32_t>(prng.next() % prims.size());
      perm = cube96::compose(perm, prims[pick]);
    }
    perms[r] = perm;
  }

  std::mt19937_64 rng(0x435562456ULL);
  std::uniform_int_distribution<std::uint32_t> dist(0, 0xFFFFFFFFu);

  long long accumulator = 0;
  for (std::size_t sample = 0; sample < samples; ++sample) {
    std::array<std::uint8_t, cube96::kBlockBytes> plain{};
    for (std::size_t i = 0; i < cube96::kBlockBytes; i += 4) {
      std::uint32_t word = dist(rng);
      for (std::size_t j = 0; j < 4 && (i + j) < cube96::kBlockBytes; ++j) {
        plain[i + j] = static_cast<std::uint8_t>((word >> (8 * j)) & 0xFFu);
      }
    }

    auto state = partial_encrypt(plain, rounds, material, perms);
    int in_parity = parity_mask(plain, mask_in);
    int out_parity = parity_mask(state, mask_out);
    accumulator += (in_parity == out_parity) ? 1 : -1;
  }

  double correlation = static_cast<double>(accumulator) / static_cast<double>(samples);
  double bias = correlation / 2.0;

  std::cout << std::fixed << std::setprecision(6)
            << "Correlation ≈ " << correlation
            << ", bias ≈ " << bias
            << " after " << rounds << " rounds using " << samples
            << " samples\n";

  return 0;
}
