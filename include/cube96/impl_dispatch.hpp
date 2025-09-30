// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include "cube96/types.hpp"

namespace cube96 {

// Fast: table-driven AES S-box, best for environments without timing
// constraints on memory access.
void sub_bytes_fast(std::uint8_t state[kBlockBytes]);
void inv_sub_bytes_fast(std::uint8_t state[kBlockBytes]);

// Hardened: bitsliced AES S-box for constant-time behaviour.
void sub_bytes_hardened(std::uint8_t state[kBlockBytes]);
void inv_sub_bytes_hardened(std::uint8_t state[kBlockBytes]);

} // namespace cube96
