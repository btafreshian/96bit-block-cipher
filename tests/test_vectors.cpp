#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cube96/cipher.hpp"

namespace {

template <std::size_t N>
bool parse_hex(const std::string &hex, std::array<std::uint8_t, N> &out) {
  if (hex.size() != N * 2) {
    return false;
  }
  for (std::size_t i = 0; i < N; ++i) {
    auto hex_value = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
      if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
      return -1;
    };
    int hi = hex_value(hex[2 * i]);
    int lo = hex_value(hex[2 * i + 1]);
    if (hi < 0 || lo < 0) {
      return false;
    }
    out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return true;
}

struct Vector {
  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> plain{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> cipher{};
};

} // namespace

int main() {
#if defined(CUBE96_LAYOUT_ROWMAJOR)
  const std::string kat_path = std::string(CUBE96_PROJECT_ROOT) +
                               "/vectors/cube96_kats_rowmajor.csv";
#else
  const std::string kat_path = std::string(CUBE96_PROJECT_ROOT) +
                               "/vectors/cube96_kats_zslice.csv";
#endif

  std::ifstream kat_file(kat_path);
  if (!kat_file) {
    std::cerr << "Unable to open KAT file: " << kat_path << "\n";
    return 1;
  }

  std::string line;
  if (!std::getline(kat_file, line)) {
    std::cerr << "KAT file is empty: " << kat_path << "\n";
    return 1;
  }

  std::vector<Vector> vectors;
  while (std::getline(kat_file, line)) {
    if (line.empty()) {
      continue;
    }
    std::stringstream ss(line);
    std::string key_hex;
    std::string plain_hex;
    std::string cipher_hex;
    if (!std::getline(ss, key_hex, ',')) {
      continue;
    }
    if (!std::getline(ss, plain_hex, ',')) {
      continue;
    }
    if (!std::getline(ss, cipher_hex, ',')) {
      continue;
    }
    Vector vec;
    if (!parse_hex(key_hex, vec.key)) {
      std::cerr << "Invalid key hex in KAT: " << key_hex << "\n";
      return 1;
    }
    if (!parse_hex(plain_hex, vec.plain)) {
      std::cerr << "Invalid plaintext hex in KAT: " << plain_hex << "\n";
      return 1;
    }
    if (!parse_hex(cipher_hex, vec.cipher)) {
      std::cerr << "Invalid ciphertext hex in KAT: " << cipher_hex << "\n";
      return 1;
    }
    vectors.push_back(vec);
  }

  if (vectors.empty()) {
    std::cerr << "No KAT entries found in " << kat_path << "\n";
    return 1;
  }

  std::vector<cube96::CubeCipher::Impl> implementations;
  if (cube96::CubeCipher::hasFastImpl()) {
    implementations.push_back(cube96::CubeCipher::Impl::Fast);
  }
  if (cube96::CubeCipher::hasHardenedImpl()) {
    implementations.push_back(cube96::CubeCipher::Impl::Hardened);
  }

  if (implementations.empty()) {
    std::cerr << "No cipher implementations available for testing\n";
    return 1;
  }

  for (const auto &vec : vectors) {
    for (auto impl : implementations) {
      cube96::CubeCipher cipher(impl);
      cipher.setKey(vec.key.data());
      std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> out{};
      cipher.encryptBlock(vec.plain.data(), out.data());
      if (out != vec.cipher) {
        std::cerr << "Ciphertext mismatch for implementation "
                  << static_cast<int>(impl) << "\n";
        return 1;
      }
      std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> recovered{};
      cipher.decryptBlock(vec.cipher.data(), recovered.data());
      if (recovered != vec.plain) {
        std::cerr << "Decrypt mismatch for implementation "
                  << static_cast<int>(impl) << "\n";
        return 1;
      }
    }
  }

  std::cout << "test_vectors: OK\n";
  return 0;
}
