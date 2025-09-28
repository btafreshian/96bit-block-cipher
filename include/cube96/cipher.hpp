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

  explicit CubeCipher(Impl impl = Impl::Fast);

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
