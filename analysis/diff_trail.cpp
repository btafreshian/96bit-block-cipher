#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cube96/endian.hpp"
#include "cube96/key_schedule.hpp"
#include "cube96/perm.hpp"
#include "cube96/sbox.hpp"
#include "cube96/types.hpp"

namespace {

struct Transition {
  std::uint8_t output;
  double weight; // -log2(probability)
  std::uint16_t count;
};

bool parse_hex_block(const std::string &hex,
                     std::array<std::uint8_t, cube96::kBlockBytes> &out) {
  if (hex.size() != cube96::kBlockBytes * 2) {
    return false;
  }
  for (std::size_t i = 0; i < cube96::kBlockBytes; ++i) {
    char hi = hex[2 * i];
    char lo = hex[2 * i + 1];
    auto hex_to_int = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
      if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
      return -1;
    };
    int hi_val = hex_to_int(hi);
    int lo_val = hex_to_int(lo);
    if (hi_val < 0 || lo_val < 0) {
      return false;
    }
    out[i] = static_cast<std::uint8_t>((hi_val << 4) | lo_val);
  }
  return true;
}

struct Context {
  int rounds;
  int branch_limit;
  const std::array<std::vector<Transition>, 256> &transitions;
  const std::array<cube96::Permutation, cube96::kRoundCount> &perms;
  std::vector<std::array<std::uint8_t, cube96::kBlockBytes>> working;
  std::vector<std::array<std::uint8_t, cube96::kBlockBytes>> best;
  double best_weight;
};

void search_round(Context &ctx, int round_idx,
                  const std::array<std::uint8_t, cube96::kBlockBytes> &input,
                  double weight);

void enumerate_bytes(Context &ctx, int round_idx,
                     const std::array<std::uint8_t, cube96::kBlockBytes> &input,
                     std::array<std::uint8_t, cube96::kBlockBytes> &sb_out,
                     std::size_t byte_idx, double weight) {
  if (byte_idx == cube96::kBlockBytes) {
    std::array<std::uint8_t, cube96::kBlockBytes> next{};
    cube96::apply_permutation(ctx.perms[round_idx], sb_out.data(), next.data());
    ctx.working[round_idx + 1] = next;
    search_round(ctx, round_idx + 1, next, weight);
    return;
  }

  std::uint8_t dx = input[byte_idx];
  if (dx == 0) {
    sb_out[byte_idx] = 0;
    enumerate_bytes(ctx, round_idx, input, sb_out, byte_idx + 1, weight);
    return;
  }

  const auto &options = ctx.transitions[dx];
  std::size_t limit = options.size();
  if (ctx.branch_limit > 0 && static_cast<int>(limit) > ctx.branch_limit) {
    limit = static_cast<std::size_t>(ctx.branch_limit);
  }
  for (std::size_t i = 0; i < limit; ++i) {
    double new_weight = weight + options[i].weight;
    if (new_weight >= ctx.best_weight) {
      continue;
    }
    sb_out[byte_idx] = options[i].output;
    enumerate_bytes(ctx, round_idx, input, sb_out, byte_idx + 1, new_weight);
  }
}

void search_round(Context &ctx, int round_idx,
                  const std::array<std::uint8_t, cube96::kBlockBytes> &input,
                  double weight) {
  ctx.working[round_idx] = input;
  if (round_idx == ctx.rounds) {
    if (weight < ctx.best_weight) {
      ctx.best_weight = weight;
      ctx.best = ctx.working;
    }
    return;
  }

  std::array<std::uint8_t, cube96::kBlockBytes> sb_out{};
  enumerate_bytes(ctx, round_idx, input, sb_out, 0, weight);
}

std::array<std::vector<Transition>, 256>
prepare_transitions(int branch_limit) {
  std::array<std::vector<Transition>, 256> transitions;
  for (int dx = 0; dx < 256; ++dx) {
    std::array<int, 256> counts{};
    counts.fill(0);
    for (int x = 0; x < 256; ++x) {
      std::uint8_t dy = cube96::AES_SBOX[x] ^ cube96::AES_SBOX[x ^ dx];
      ++counts[dy];
    }
    auto &vec = transitions[dx];
    if (dx == 0) {
      vec.push_back({0, 0.0, 256});
      continue;
    }
    for (int dy = 0; dy < 256; ++dy) {
      int count = counts[dy];
      if (count == 0) {
        continue;
      }
      double prob = static_cast<double>(count) / 256.0;
      double weight = -std::log2(prob);
      vec.push_back({static_cast<std::uint8_t>(dy), weight,
                     static_cast<std::uint16_t>(count)});
    }
    std::sort(vec.begin(), vec.end(), [](const Transition &a, const Transition &b) {
      if (a.weight == b.weight) {
        return a.output < b.output;
      }
      return a.weight < b.weight;
    });
    if (branch_limit > 0 && static_cast<int>(vec.size()) > branch_limit) {
      vec.resize(static_cast<std::size_t>(branch_limit));
    }
  }
  return transitions;
}

void print_state(const std::array<std::uint8_t, cube96::kBlockBytes> &state) {
  for (std::size_t i = 0; i < cube96::kBlockBytes; ++i) {
    std::cout << std::hex << std::setfill('0') << std::setw(2)
              << static_cast<int>(state[i]);
  }
  std::cout << std::dec;
}

} // namespace

int main(int argc, char **argv) {
  int rounds = 4;
  int branch_limit = 8;
  std::array<std::uint8_t, cube96::kBlockBytes> key{};
  std::array<std::uint8_t, cube96::kBlockBytes> input_diff{};
  input_diff.fill(0);
  input_diff[0] = 0x01;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--rounds" && i + 1 < argc) {
      rounds = std::stoi(argv[++i]);
    } else if (arg == "--branch" && i + 1 < argc) {
      branch_limit = std::stoi(argv[++i]);
    } else if (arg == "--key" && i + 1 < argc) {
      if (!parse_hex_block(argv[++i], key)) {
        std::cerr << "Invalid key hex string\n";
        return 1;
      }
    } else if (arg == "--diff" && i + 1 < argc) {
      if (!parse_hex_block(argv[++i], input_diff)) {
        std::cerr << "Invalid input difference hex string\n";
        return 1;
      }
    } else if (arg == "--help") {
      std::cout << "Usage: cube96_diff_trails [--rounds N] [--branch N]"
                   " [--key HEX] [--diff HEX]\n";
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

  if (branch_limit < 1) {
    branch_limit = 1;
  }

  auto transitions = prepare_transitions(branch_limit);

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

  Context ctx{rounds, branch_limit, transitions, perms,
              std::vector<std::array<std::uint8_t, cube96::kBlockBytes>>(rounds + 1),
              std::vector<std::array<std::uint8_t, cube96::kBlockBytes>>(rounds + 1),
              std::numeric_limits<double>::infinity()};

  search_round(ctx, 0, input_diff, 0.0);

  if (!std::isfinite(ctx.best_weight)) {
    std::cerr << "No trail found with the given parameters\n";
    return 1;
  }

  double probability = std::pow(2.0, -ctx.best_weight);
  std::cout << "Best trail over " << rounds << " rounds:" << "\n";
  for (int r = 0; r < rounds; ++r) {
    std::cout << "  Round " << r << " input diff: ";
    print_state(ctx.best[r]);
    std::cout << "\n";
  }
  std::cout << "  After round " << rounds << " permutation: ";
  print_state(ctx.best[rounds]);
  std::cout << "\n";
  std::cout << std::fixed << std::setprecision(6)
            << "  Trail probability ≈ " << probability
            << " (weight = " << ctx.best_weight << ")\n";

  return 0;
}
