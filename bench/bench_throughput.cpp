#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "cube96/cipher.hpp"

namespace {

void run_bench(cube96::CubeCipher::Impl impl, std::size_t bytes) {
  cube96::CubeCipher cipher(impl);
  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{};
  for (std::size_t i = 0; i < key.size(); ++i) {
    key[i] = static_cast<std::uint8_t>(i * 11u + 7u);
  }
  cipher.setKey(key.data());

  std::vector<std::uint8_t> buffer(bytes);
  std::mt19937 rng(12345u);
  std::uniform_int_distribution<int> dist(0, 255);
  for (auto &b : buffer) {
    b = static_cast<std::uint8_t>(dist(rng));
  }

  std::vector<std::uint8_t> out(bytes);
  const std::size_t blocks = bytes / cube96::CubeCipher::BlockBytes;

  auto start = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < blocks; ++i) {
    cipher.encryptBlock(buffer.data() + i * cube96::CubeCipher::BlockBytes,
                        out.data() + i * cube96::CubeCipher::BlockBytes);
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
  double mbps = mb / elapsed.count();
  std::cout << (impl == cube96::CubeCipher::Impl::Fast ? "Fast" : "Hardened")
            << " impl: " << std::fixed << std::setprecision(2) << mbps
            << " MiB/s in " << elapsed.count() << " s\n";
}

} // namespace

int main(int argc, char **argv) {
  std::size_t bytes = 64ull * 1024ull * 1024ull;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--bytes" && i + 1 < argc) {
      bytes = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--blocks" && i + 1 < argc) {
      std::size_t blocks = static_cast<std::size_t>(std::stoull(argv[++i]));
      bytes = blocks * cube96::CubeCipher::BlockBytes;
    } else if (arg == "--help") {
      std::cout << "Usage: cube96_bench [--bytes N] [--blocks N]\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return 1;
    }
  }
  if (bytes == 0 || bytes % cube96::CubeCipher::BlockBytes != 0) {
    std::cerr << "Byte count must be a positive multiple of block size ("
              << cube96::CubeCipher::BlockBytes << ")\n";
    return 1;
  }
  run_bench(cube96::CubeCipher::Impl::Fast, bytes);
  run_bench(cube96::CubeCipher::Impl::Hardened, bytes);
  return 0;
}
