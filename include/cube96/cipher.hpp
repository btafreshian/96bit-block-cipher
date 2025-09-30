// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "cube96/types.hpp"

namespace cube96 {

class CubeCipher {
public:
  static constexpr std::size_t BlockBytes = kBlockBytes;
  static constexpr std::size_t KeyBytes   = kKeyBytes;

  enum class Impl { Fast, Hardened };

  static constexpr Impl DefaultImpl =
#if defined(CUBE96_FORCE_CONSTANT_TIME) || defined(CUBE96_DISABLE_FAST_IMPL)
      Impl::Hardened
#else
      Impl::Fast
#endif
  ;

  static constexpr bool hasFastImpl() {
#if defined(CUBE96_DISABLE_FAST_IMPL)
    return false;
#else
    return true;
#endif
  }

  static constexpr bool hasHardenedImpl() { return true; }

  // Choose Impl::Fast for table-based S-boxes and Impl::Hardened for the
  // bitsliced constant-time circuit.  Both options share the same key schedule
  // and permutation derivation logic.  Builds configured with
  // CUBE96_FORCE_CONSTANT_TIME force DefaultImpl to Hardened and disable Fast
  // dispatch, exposing the policy through hasFastImpl().

  explicit CubeCipher(Impl impl = DefaultImpl);

  void setKey(const std::uint8_t key[KeyBytes]);

  void encryptBlock(const std::uint8_t in[BlockBytes],
                    std::uint8_t out[BlockBytes]) const;

  void decryptBlock(const std::uint8_t in[BlockBytes],
                    std::uint8_t out[BlockBytes]) const;

private:
  std::array<RoundKey, kRoundCount> round_keys_{};
  RoundKey                          rk_post_{};
  std::array<Permutation, kRoundCount> perm_{};
  std::array<Permutation, kRoundCount> inv_perm_{};

  Impl impl_;
};

} // namespace cube96
