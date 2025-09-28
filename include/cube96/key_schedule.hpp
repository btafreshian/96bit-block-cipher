#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "cube96/types.hpp"

namespace cube96 {

struct DerivedMaterial {
  std::array<RoundKey, kRoundCount> round_keys{};
  std::array<std::array<std::uint8_t, 8>, kRoundCount> perm_seeds{};
  RoundKey post_whitening{};
};

DerivedMaterial derive_material(const std::uint8_t key[kKeyBytes]);

// SHA-256 interface exposed for unit tests.
struct Sha256Digest {
  std::uint32_t h[8];
};

Sha256Digest sha256(const std::uint8_t *data, std::size_t len);
void hmac_sha256(const std::uint8_t *key, std::size_t key_len,
                 const std::uint8_t *data, std::size_t data_len,
                 std::uint8_t out[32]);
void hkdf_expand(const std::uint8_t prk[32], const std::uint8_t *info,
                 std::size_t info_len, std::uint8_t *out, std::size_t out_len);

} // namespace cube96
