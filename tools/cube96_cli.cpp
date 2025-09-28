#include <array>
#include <cctype>
#include <iostream>
#include <string>

#include "cube96/cipher.hpp"

namespace {

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

bool parse_hex(const std::string &hex,
               std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> &out) {
  if (hex.size() != out.size() * 2) {
    return false;
  }
  for (std::size_t i = 0; i < out.size(); ++i) {
    int hi = hex_value(hex[2 * i]);
    int lo = hex_value(hex[2 * i + 1]);
    if (hi < 0 || lo < 0) {
      return false;
    }
    out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return true;
}

void print_hex(const std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> &data) {
  static const char *digits = "0123456789abcdef";
  for (auto b : data) {
    std::cout << digits[b >> 4] << digits[b & 0x0F];
  }
  std::cout << '\n';
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: cube96 <enc|dec> <hex-key-24> <hex-data-24>\n";
    return 1;
  }

  std::string mode = argv[1];
  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> input{};

  if (!parse_hex(argv[2], key) || !parse_hex(argv[3], input)) {
    std::cerr << "Invalid hex input (expected 24 hex characters).\n";
    return 1;
  }

  cube96::CubeCipher cipher;
  cipher.setKey(key.data());

  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> output{};

  if (mode == "enc") {
    cipher.encryptBlock(input.data(), output.data());
  } else if (mode == "dec") {
    cipher.decryptBlock(input.data(), output.data());
  } else {
    std::cerr << "Unknown mode: " << mode << '\n';
    return 1;
  }

  print_hex(output);
  return 0;
}
