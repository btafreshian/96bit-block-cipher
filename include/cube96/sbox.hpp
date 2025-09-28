#pragma once

#include <cstdint>

namespace cube96 {

extern const std::uint8_t AES_SBOX[256];
extern const std::uint8_t AES_INV_SBOX[256];

std::uint8_t aes_sbox_bitsliced(std::uint8_t x);
std::uint8_t aes_inv_sbox_bitsliced(std::uint8_t x);

} // namespace cube96
