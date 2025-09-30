// SPDX-License-Identifier: MIT

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cube96/cipher.hpp"

namespace {

struct KatCase {
  std::string_view name;
  std::string_view key_hex;
  std::string_view plaintext_hex;
};

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

template <std::size_t N>
bool parse_hex(std::string_view hex, std::array<std::uint8_t, N> &out) {
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
std::string to_hex(const std::array<std::uint8_t, N> &data) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string out;
  out.reserve(data.size() * 2);
  for (auto b : data) {
    out.push_back(kDigits[b >> 4]);
    out.push_back(kDigits[b & 0x0F]);
  }
  return out;
}

std::optional<std::ofstream> open_output_file(const char *path) {
  std::ofstream file(path, std::ios::out | std::ios::trunc);
  if (!file) {
    std::cerr << "Failed to open output file '" << path << "'\n";
    return std::nullopt;
  }
  return file;
}

}  // namespace

int main(int argc, char **argv) {
  std::ostream *stream = &std::cout;
  std::optional<std::ofstream> file;
  if (argc == 2) {
    file = open_output_file(argv[1]);
    if (!file) {
      return 1;
    }
    stream = &*file;
  } else if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [output.csv]\n";
    return 1;
  }

  static const std::vector<KatCase> kCases = {
      {"kat0_zero", "000000000000000000000000", "000000000000000000000000"},
      {"kat1_key_ff", "ffffffffffffffffffffffff", "000000000000000000000000"},
      {"kat2_increment", "000102030405060708090a0b", "0c0d0e0f1011121314151617"},
      {"kat3_stride", "00112233445566778899aabb", "ccddee00ff11223344556677"},
      {"kat4_mixed", "0123456789abcdef00112233", "445566778899aabbccddeeff"},
      {"kat5_descend", "fedcba9876543210ffeeddcc", "bbaa99887766554433221100"},
      {"kat6_pattern", "0f1e2d3c4b5a69788796a5b4", "c3d2e1f0ffeeddccbbaa9988"},
      {"kat7_sparse", "800000000000000000000001", "000000000000000000000001"},
  };

  cube96::CubeCipher cipher;

  *stream << "# SPDX-License-Identifier: MIT\n";
  *stream << "name,key,plaintext,ciphertext\n";

  for (const auto &kat : kCases) {
    std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
    std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> plain{};

    if (!parse_hex(kat.key_hex, key)) {
      std::cerr << "Invalid key hex in " << kat.name << "\n";
      return 1;
    }
    if (!parse_hex(kat.plaintext_hex, plain)) {
      std::cerr << "Invalid plaintext hex in " << kat.name << "\n";
      return 1;
    }

    cipher.setKey(key.data());
    std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> cipher_text{};
    cipher.encryptBlock(plain.data(), cipher_text.data());

    *stream << kat.name << ',' << to_hex(key) << ',' << to_hex(plain) << ','
            << to_hex(cipher_text) << '\n';
  }

  stream->flush();
  if (!stream->good()) {
    std::cerr << "Failed to write vectors\n";
    return 1;
  }
  return 0;
}
