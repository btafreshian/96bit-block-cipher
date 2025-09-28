#include <array>
#include <iostream>
#include <random>

#include "cube96/cipher.hpp"

int main() {
  std::mt19937_64 rng(0xC0FFEEu);
  std::uniform_int_distribution<int> dist(0, 255);

  cube96::CubeCipher fast(cube96::CubeCipher::Impl::Fast);
  cube96::CubeCipher hard(cube96::CubeCipher::Impl::Hardened);

  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> plain{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> cipher_fast{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> cipher_hard{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> recovered{};

  const std::size_t iterations = 50000;
  for (std::size_t iter = 0; iter < iterations; ++iter) {
    for (auto &b : key) {
      b = static_cast<std::uint8_t>(dist(rng));
    }
    for (auto &b : plain) {
      b = static_cast<std::uint8_t>(dist(rng));
    }

    fast.setKey(key.data());
    hard.setKey(key.data());

    fast.encryptBlock(plain.data(), cipher_fast.data());
    fast.decryptBlock(cipher_fast.data(), recovered.data());
    if (recovered != plain) {
      std::cerr << "Fast roundtrip mismatch at iteration " << iter << "\n";
      return 1;
    }

    hard.encryptBlock(plain.data(), cipher_hard.data());
    if (cipher_hard != cipher_fast) {
      std::cerr << "Implementation mismatch at iteration " << iter << "\n";
      return 1;
    }

    hard.decryptBlock(cipher_fast.data(), recovered.data());
    if (recovered != plain) {
      std::cerr << "Hardened roundtrip mismatch at iteration " << iter << "\n";
      return 1;
    }
  }

  std::cout << "test_roundtrip: OK\n";
  return 0;
}
