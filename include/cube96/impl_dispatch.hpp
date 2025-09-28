#pragma once

#include <cstdint>

#include "cube96/types.hpp"

namespace cube96 {

void sub_bytes_fast(std::uint8_t state[kBlockBytes]);
void inv_sub_bytes_fast(std::uint8_t state[kBlockBytes]);

void sub_bytes_hardened(std::uint8_t state[kBlockBytes]);
void inv_sub_bytes_hardened(std::uint8_t state[kBlockBytes]);

} // namespace cube96
