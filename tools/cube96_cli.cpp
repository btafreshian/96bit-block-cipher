// SPDX-License-Identifier: MIT

#include <array>
#include <cctype>
#include <iostream>
#include <string>

#include "cube96/cipher.hpp"

namespace {

constexpr char kWarning[] =
    "Research cipher — NOT FOR PRODUCTION. Key size chosen for tractability, not "
    "security.";

constexpr int kExitSuccess = 0;
constexpr int kExitUsage = 64;
constexpr int kExitHexError = 65;
constexpr int kExitModeError = 66;

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

template <std::size_t N>
bool parse_hex(const std::string &hex, std::array<std::uint8_t, N> &out) {
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

template <std::size_t N>
bool parse_hex_argument(const std::string &hex, const char *label,
                        std::array<std::uint8_t, N> &out) {
  if (!parse_hex(hex, out)) {
    std::cerr << "Invalid " << label << " (expected " << (out.size() * 2)
              << " hex characters)." << '\n';
    return false;
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

int print_usage(const char *prog_name) {
  std::cerr << "Usage: " << prog_name
            << " <enc|dec> <hex-key-24> <hex-data-24>" << '\n';
  return kExitUsage;
}

} // namespace

int main(int argc, char **argv) {
  std::cerr << kWarning << '\n';

  if (argc != 4) {
    return print_usage(argv[0]);
  }

  std::string mode = argv[1];
  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> input{};

  if (!parse_hex_argument(argv[2], "key", key)) {
    return kExitHexError;
  }

  if (mode != "enc" && mode != "dec") {
    std::cerr << "Unknown mode: " << mode << '\n';
    print_usage(argv[0]);
    return kExitModeError;
  }

  const char *data_label = mode == "enc" ? "plaintext" : "ciphertext";
  if (!parse_hex_argument(argv[3], data_label, input)) {
    return kExitHexError;
  }

  cube96::CubeCipher cipher;
  cipher.setKey(key.data());

  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> output{};

  if (mode == "enc") {
    cipher.encryptBlock(input.data(), output.data());
  } else if (mode == "dec") {
    cipher.decryptBlock(input.data(), output.data());
  }

  print_hex(output);
  return kExitSuccess;
}
