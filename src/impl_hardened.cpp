#include "cube96/impl_dispatch.hpp"

#include "cube96/sbox.hpp"
#include "cube96/types.hpp"

namespace cube96 {

void sub_bytes_hardened(std::uint8_t state[kBlockBytes]) {
  for (std::size_t i = 0; i < kBlockBytes; ++i) {
    state[i] = aes_sbox_bitsliced(state[i]);
  }
}

void inv_sub_bytes_hardened(std::uint8_t state[kBlockBytes]) {
  for (std::size_t i = 0; i < kBlockBytes; ++i) {
    state[i] = aes_inv_sbox_bitsliced(state[i]);
  }
}

} // namespace cube96
