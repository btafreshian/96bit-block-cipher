#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "cube96/cipher.hpp"

int main() {
  std::mt19937_64 rng(0xC0FFEEu);
  std::uniform_int_distribution<int> dist(0, 255);

  std::vector<cube96::CubeCipher::Impl> implementations;
  if (cube96::CubeCipher::hasFastImpl()) {
    implementations.push_back(cube96::CubeCipher::Impl::Fast);
  }
  if (cube96::CubeCipher::hasHardenedImpl()) {
    implementations.push_back(cube96::CubeCipher::Impl::Hardened);
  }

  if (implementations.empty()) {
    std::cerr << "No implementations enabled\n";
    return 1;
  }

  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> plain{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> recovered{};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> baseline{};

  // Default to a 10k-sample sweep to balance coverage with CI runtime.  Allow
  // overriding through an environment variable so developers can dial the
  // iteration count up when investigating discrepancies locally.
  std::size_t iterations = 10000;
  if (const char *env = std::getenv("CUBE96_TEST_ITERATIONS")) {
    char *end = nullptr;
    unsigned long parsed = std::strtoul(env, &end, 10);
    if (end != env && *end == '\0' && parsed > 0) {
      iterations = static_cast<std::size_t>(parsed);
    }
  }
  for (std::size_t iter = 0; iter < iterations; ++iter) {
    for (auto &b : key) {
      b = static_cast<std::uint8_t>(dist(rng));
    }
    for (auto &b : plain) {
      b = static_cast<std::uint8_t>(dist(rng));
    }

    bool have_baseline = false;
    for (auto impl : implementations) {
      cube96::CubeCipher cipher(impl);
      cipher.setKey(key.data());
      std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> cipher_text{};
      cipher.encryptBlock(plain.data(), cipher_text.data());
      if (!have_baseline) {
        baseline = cipher_text;
        have_baseline = true;
      } else if (cipher_text != baseline) {
        std::cerr << "Implementation mismatch at iteration " << iter << "\n";
        return 1;
      }
      cipher.decryptBlock(cipher_text.data(), recovered.data());
      if (recovered != plain) {
        std::cerr << "Roundtrip mismatch at iteration " << iter << " for impl "
                  << static_cast<int>(impl) << "\n";
        return 1;
      }
    }
  }

  std::cout << "test_roundtrip: OK\n";
  return 0;
}
