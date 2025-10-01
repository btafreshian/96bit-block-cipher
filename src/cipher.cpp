// SPDX-License-Identifier: MIT

#include "cube96/cipher.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

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

CubeCipher::CubeCipher(Impl impl) : impl_(impl) {
#if defined(CUBE96_FORCE_CONSTANT_TIME) || defined(CUBE96_DISABLE_FAST_IMPL)
  if (impl == Impl::Fast) {
    throw std::invalid_argument("Fast implementation disabled at build time");
  }
  impl_ = Impl::Hardened;
#endif
}

void CubeCipher::setKey(const std::uint8_t key[KeyBytes]) {
  DerivedMaterial material = derive_material(key);
  round_keys_ = material.round_keys;
  rk_post_ = material.post_whitening;

  const auto &primitives = primitive_set();
  const std::size_t primitive_count = primitives.size();
  const std::uint64_t limit =
      (std::numeric_limits<std::uint64_t>::max() / primitive_count) * primitive_count;

  for (std::size_t r = 0; r < kRoundCount; ++r) {
    std::uint64_t seed = load_be64(material.perm_seeds[r].data());
    SplitMix64 prng(seed);
    Permutation perm = identity_permutation();
    for (int step = 0; step < 12; ++step) {
      std::uint64_t draw = prng.next();
      while (draw >= limit) {
        draw = prng.next();
      }
      const std::size_t pick = static_cast<std::size_t>(draw % primitive_count);
      perm = compose(perm, primitives[pick]);
    }
    perm_[r] = perm;
    inv_perm_[r] = invert(perm);
  }
}

void CubeCipher::encryptBlock(const std::uint8_t in[BlockBytes],
                              std::uint8_t out[BlockBytes]) const {
  std::uint8_t buf_a[BlockBytes];
  std::uint8_t buf_b[BlockBytes];
  std::memcpy(buf_a, in, BlockBytes);
  std::uint8_t *cur = buf_a;
  std::uint8_t *next = buf_b;

#if !defined(CUBE96_DISABLE_FAST_IMPL)
  const bool use_fast = (impl_ == Impl::Fast);
#else
  const bool use_fast = false;
#endif

  for (std::size_t r = 0; r < kRoundCount; ++r) {
    for (std::size_t i = 0; i < BlockBytes; ++i) {
      cur[i] ^= round_keys_[r][i];
    }

    if (use_fast) {
#if !defined(CUBE96_DISABLE_FAST_IMPL)
      sub_bytes_fast(cur);
#endif
    } else {
      sub_bytes_hardened(cur);
    }

    if (use_fast) {
#if !defined(CUBE96_DISABLE_FAST_IMPL)
      apply_permutation(perm_[r], cur, next);
#endif
    } else {
      apply_permutation_ct(perm_[r], cur, next);
    }
    std::swap(cur, next);
  }

  for (std::size_t i = 0; i < BlockBytes; ++i) {
    cur[i] ^= rk_post_[i];
  }
  std::memcpy(out, cur, BlockBytes);
}

void CubeCipher::decryptBlock(const std::uint8_t in[BlockBytes],
                              std::uint8_t out[BlockBytes]) const {
  std::uint8_t buf_a[BlockBytes];
  std::uint8_t buf_b[BlockBytes];
  std::memcpy(buf_a, in, BlockBytes);
  std::uint8_t *cur = buf_a;
  std::uint8_t *next = buf_b;

#if !defined(CUBE96_DISABLE_FAST_IMPL)
  const bool use_fast = (impl_ == Impl::Fast);
#else
  const bool use_fast = false;
#endif

  for (std::size_t i = 0; i < BlockBytes; ++i) {
    cur[i] ^= rk_post_[i];
  }

  for (int r = static_cast<int>(kRoundCount) - 1; r >= 0; --r) {
    if (use_fast) {
#if !defined(CUBE96_DISABLE_FAST_IMPL)
      apply_permutation(inv_perm_[r], cur, next);
#endif
    } else {
      apply_permutation_ct(inv_perm_[r], cur, next);
    }
    std::swap(cur, next);

    if (use_fast) {
#if !defined(CUBE96_DISABLE_FAST_IMPL)
      inv_sub_bytes_fast(cur);
#endif
    } else {
      inv_sub_bytes_hardened(cur);
    }

    for (std::size_t i = 0; i < BlockBytes; ++i) {
      cur[i] ^= round_keys_[r][i];
    }
  }

  std::memcpy(out, cur, BlockBytes);
}

} // namespace cube96
