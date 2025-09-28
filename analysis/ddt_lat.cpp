#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "cube96/sbox.hpp"

namespace {

bool write_matrix_csv(const std::string &path,
                      const std::vector<std::array<int, 256>> &matrix) {
  std::ofstream file(path);
  if (!file) {
    std::cerr << "Failed to open " << path << " for writing\n";
    return false;
  }

  file << "dx";
  for (int col = 0; col < 256; ++col) {
    file << "," << col;
  }
  file << "\n";

  for (std::size_t row = 0; row < matrix.size(); ++row) {
    file << row;
    for (int col = 0; col < 256; ++col) {
      file << ',' << matrix[row][col];
    }
    file << "\n";
  }

  return true;
}

int parity8(std::uint8_t x) {
  x ^= static_cast<std::uint8_t>(x >> 4);
  x ^= static_cast<std::uint8_t>(x >> 2);
  x ^= static_cast<std::uint8_t>(x >> 1);
  return x & 1;
}

} // namespace

int main(int argc, char **argv) {
  std::string ddt_path = "ddt.csv";
  std::string lat_path = "lat.csv";
  if (argc > 1) {
    ddt_path = argv[1];
  }
  if (argc > 2) {
    lat_path = argv[2];
  }

  std::vector<std::array<int, 256>> ddt(256);
  for (int dx = 0; dx < 256; ++dx) {
    ddt[dx].fill(0);
    for (int x = 0; x < 256; ++x) {
      std::uint8_t dy = cube96::AES_SBOX[x] ^ cube96::AES_SBOX[x ^ dx];
      ++ddt[dx][dy];
    }
  }

  int max_uniformity = 0;
  for (int dx = 1; dx < 256; ++dx) {
    for (int dy = 0; dy < 256; ++dy) {
      if (ddt[dx][dy] > max_uniformity) {
        max_uniformity = ddt[dx][dy];
      }
    }
  }

  if (!write_matrix_csv(ddt_path, ddt)) {
    return 1;
  }

  std::vector<std::array<int, 256>> lat(256);
  for (int a = 0; a < 256; ++a) {
    lat[a].fill(0);
    for (int b = 0; b < 256; ++b) {
      int sum = 0;
      for (int x = 0; x < 256; ++x) {
        int in_parity = parity8(static_cast<std::uint8_t>(a & x));
        int out_parity = parity8(static_cast<std::uint8_t>(b & cube96::AES_SBOX[x]));
        sum += (in_parity == out_parity) ? 1 : -1;
      }
      lat[a][b] = sum;
    }
  }

  int max_bias = 0;
  for (int a = 0; a < 256; ++a) {
    if (a == 0) {
      continue;
    }
    for (int b = 0; b < 256; ++b) {
      if (b == 0) {
        continue;
      }
      int bias = std::abs(lat[a][b]);
      if (bias > max_bias) {
        max_bias = bias;
      }
    }
  }

  if (!write_matrix_csv(lat_path, lat)) {
    return 1;
  }

  std::cout << "DDT written to " << ddt_path
            << ", max differential uniformity = " << max_uniformity << "\n";
  std::cout << "LAT written to " << lat_path
            << ", max absolute bias = " << max_bias << "/256" << "\n";
  return 0;
}
