// SPDX-License-Identifier: MIT

#include "cube96/cipher.hpp"

#include <algorithm>
#include <cstring>

#include "cube96/impl_dispatch.hpp"
#include "cube96/key_schedule.hpp"
#include "cube96/perm.hpp"
#include "cube96/endian.hpp"

namespace cube96 {

// Round structure: the cipher performs kRoundCount iterations of key addition,
// byte-wise SubBytes, and a permutation whose shape is derived from the key.
// The caller selects between the fast table S-box and the bitsliced
// constant-time path through the Impl enum, and the same choice governs the
// permutation helper so that both halves of the round adhere to the selected
// side-channel trade-off.

CubeCipher::CubeCipher(Impl impl) : impl_(impl) {}

void CubeCipher::setKey(const std::uint8_t key[KeyBytes]) {
  DerivedMaterial material = derive_material(key);
  round_keys_ = material.round_keys;
  rk_post_ = material.post_whitening;

  const auto &primitives = primitive_set();
  for (std::size_t r = 0; r < kRoundCount; ++r) {
    std::uint64_t seed = load_be64(material.perm_seeds[r].data());
    SplitMix64 prng(seed);
    Permutation perm = identity_permutation();
    for (int step = 0; step < 12; ++step) {
      std::uint32_t pick = static_cast<std::uint32_t>(prng.next() % primitives.size());
      perm = compose(perm, primitives[pick]);
    }
    perm_[r] = perm;
    inv_perm_[r] = invert(perm);
  }
}

void CubeCipher::encryptBlock(const std::uint8_t in[BlockBytes],
                              std::uint8_t out[BlockBytes]) const {
  std::uint8_t state[BlockBytes];
  std::memcpy(state, in, BlockBytes);

  for (std::size_t r = 0; r < kRoundCount; ++r) {
    // AddRoundKey → SubBytes → bit permutation.  The S-box executes prior to
    // shuffling so diffusion spans the entire cube before the next key mix.
    for (std::size_t i = 0; i < BlockBytes; ++i) {
      state[i] ^= round_keys_[r][i];
    }
    if (impl_ == Impl::Fast) {
      sub_bytes_fast(state);
    } else {
      sub_bytes_hardened(state);
    }
    std::uint8_t tmp[BlockBytes];
    if (impl_ == Impl::Fast) {
      apply_permutation(perm_[r], state, tmp);
    } else {
      apply_permutation_ct(perm_[r], state, tmp);
    }
    std::memcpy(state, tmp, BlockBytes);
  }

  for (std::size_t i = 0; i < BlockBytes; ++i) {
    state[i] ^= rk_post_[i];
  }
  std::memcpy(out, state, BlockBytes);
}

void CubeCipher::decryptBlock(const std::uint8_t in[BlockBytes],
                              std::uint8_t out[BlockBytes]) const {
  std::uint8_t state[BlockBytes];
  std::memcpy(state, in, BlockBytes);

  for (std::size_t i = 0; i < BlockBytes; ++i) {
    state[i] ^= rk_post_[i];
  }

  for (int r = static_cast<int>(kRoundCount) - 1; r >= 0; --r) {
    std::uint8_t tmp[BlockBytes];
    if (impl_ == Impl::Fast) {
      apply_permutation(inv_perm_[r], state, tmp);
    } else {
      apply_permutation_ct(inv_perm_[r], state, tmp);
    }
    std::memcpy(state, tmp, BlockBytes);
    if (impl_ == Impl::Fast) {
      inv_sub_bytes_fast(state);
    } else {
      inv_sub_bytes_hardened(state);
    }
    for (std::size_t i = 0; i < BlockBytes; ++i) {
      state[i] ^= round_keys_[r][i];
    }
  }

  std::memcpy(out, state, BlockBytes);
}

} // namespace cube96
