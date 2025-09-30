// SPDX-License-Identifier: MIT

#include "cube96/impl_dispatch.hpp"

#include "cube96/sbox.hpp"
#include "cube96/types.hpp"

namespace cube96 {

// The table-driven implementation is tuned for speed on platforms where
// data-dependent table lookups are acceptable.  The AES S-box tables are
// shared with the hardened variant to keep the algebraic definition aligned.
void sub_bytes_fast(std::uint8_t state[kBlockBytes]) {
  for (std::size_t i = 0; i < kBlockBytes; ++i) {
    state[i] = AES_SBOX[state[i]];
  }
}

void inv_sub_bytes_fast(std::uint8_t state[kBlockBytes]) {
  for (std::size_t i = 0; i < kBlockBytes; ++i) {
    state[i] = AES_INV_SBOX[state[i]];
  }
}

} // namespace cube96
