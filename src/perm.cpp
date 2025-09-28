#include "cube96/perm.hpp"

#include <algorithm>
#include <cstring>

#include "cube96/ct_utils.hpp"
#include "cube96/types.hpp"

namespace cube96 {

std::uint64_t SplitMix64::next() {
  // Reference algorithm from Steele et al., as required by the specification.
  std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

Permutation identity_permutation() {
  Permutation p{};
  for (std::size_t i = 0; i < kPermSize; ++i) {
    p[i] = static_cast<std::uint8_t>(i);
  }
  return p;
}

Permutation compose(const Permutation &accum, const Permutation &step) {
  Permutation out{};
  for (std::size_t i = 0; i < kPermSize; ++i) {
    out[i] = step[accum[i]];
  }
  return out;
}

Permutation invert(const Permutation &p) {
  Permutation inv{};
  for (std::size_t i = 0; i < kPermSize; ++i) {
    inv[p[i]] = static_cast<std::uint8_t>(i);
  }
  return inv;
}

void apply_permutation(const Permutation &p, const std::uint8_t in[kBlockBytes],
                       std::uint8_t out[kBlockBytes]) {
  std::uint8_t tmp[kBlockBytes] = {0};
  for (std::uint8_t src = 0; src < kPermSize; ++src) {
    std::uint8_t bit = get_bit(in, src);
    set_bit(tmp, p[src], bit);
  }
  std::memcpy(out, tmp, kBlockBytes);
}

void apply_permutation_ct(const Permutation &p, const std::uint8_t in[kBlockBytes],
                          std::uint8_t out[kBlockBytes]) {
  std::uint8_t tmp[kBlockBytes] = {0};
  for (std::uint8_t src = 0; src < kPermSize; ++src) {
    std::uint8_t byte_index = byte_index_of_bit(src);
    std::uint8_t bit_pos = bit_offset_in_byte(src);
    std::uint8_t bit = static_cast<std::uint8_t>((in[byte_index] >> bit_pos) & 1u);
    std::uint8_t dst = p[src];
    std::uint8_t dst_byte = byte_index_of_bit(dst);
    std::uint8_t dst_pos = bit_offset_in_byte(dst);
    ct_write_bit(tmp, dst_byte, dst_pos, bit);
  }
  std::memcpy(out, tmp, kBlockBytes);
}

namespace {

Permutation face_rotation(std::uint8_t z, int variant) {
  // variant: 0 = 90° CW, 1 = 90° CCW, 2 = 180°
  Permutation p = identity_permutation();
  for (std::uint8_t y = 0; y < 4; ++y) {
    for (std::uint8_t x = 0; x < 4; ++x) {
      std::uint8_t nx = 0;
      std::uint8_t ny = 0;
      if (variant == 0) {
        nx = static_cast<std::uint8_t>(3 - y);
        ny = x;
      } else if (variant == 1) {
        nx = y;
        ny = static_cast<std::uint8_t>(3 - x);
      } else {
        nx = static_cast<std::uint8_t>(3 - x);
        ny = static_cast<std::uint8_t>(3 - y);
      }
      std::uint8_t src = idx_of(x, y, z);
      std::uint8_t dst = idx_of(nx, ny, z);
      p[src] = dst;
    }
  }
  return p;
}

Permutation row_cycle(std::uint8_t z, bool up) {
  Permutation p = identity_permutation();
  for (std::uint8_t y = 0; y < 4; ++y) {
    std::uint8_t ny = static_cast<std::uint8_t>((y + (up ? 1 : 3)) & 3u);
    for (std::uint8_t x = 0; x < 4; ++x) {
      std::uint8_t src = idx_of(x, y, z);
      std::uint8_t dst = idx_of(x, ny, z);
      p[src] = dst;
    }
  }
  return p;
}

Permutation column_cycle(std::uint8_t z, bool right) {
  Permutation p = identity_permutation();
  for (std::uint8_t x = 0; x < 4; ++x) {
    std::uint8_t nx = static_cast<std::uint8_t>((x + (right ? 1 : 3)) & 3u);
    for (std::uint8_t y = 0; y < 4; ++y) {
      std::uint8_t src = idx_of(x, y, z);
      std::uint8_t dst = idx_of(nx, y, z);
      p[src] = dst;
    }
  }
  return p;
}

Permutation x_slice_shift(std::uint8_t x) {
  Permutation p = identity_permutation();
  for (std::uint8_t y = 0; y < 4; ++y) {
    for (std::uint8_t z = 0; z < 6; ++z) {
      std::uint8_t nz = static_cast<std::uint8_t>((z + 1) % 6);
      std::uint8_t src = idx_of(x, y, z);
      std::uint8_t dst = idx_of(x, y, nz);
      p[src] = dst;
    }
  }
  return p;
}

Permutation y_slice_shift(std::uint8_t y) {
  Permutation p = identity_permutation();
  for (std::uint8_t x = 0; x < 4; ++x) {
    for (std::uint8_t z = 0; z < 6; ++z) {
      std::uint8_t nz = static_cast<std::uint8_t>((z + 1) % 6);
      std::uint8_t src = idx_of(x, y, z);
      std::uint8_t dst = idx_of(x, y, nz);
      p[src] = dst;
    }
  }
  return p;
}

// Primitive index layout (0-based):
//  0..17  : z-layer face rotations (CW, CCW, 180°) for z=0..5.
// 18..29  : row/column cycles (row up, row down, column right) for z=0..3.
//           Column-left cycles are omitted because applying the right-cycle
//           three times produces the same transformation, keeping the curated
//           set compact and bijective.
// 30..35  : aggregate z-shifts for x ∈ {0,1,2} followed by y ∈ {0,1,2}.
const std::array<Permutation, 36> kPrimitives = [] {
  std::array<Permutation, 36> prim{};
  std::size_t idx = 0;
  // 18 face rotations: z=0..5 with CW, CCW, 180°
  for (std::uint8_t z = 0; z < 6; ++z) {
    prim[idx++] = face_rotation(z, 0);
    prim[idx++] = face_rotation(z, 1);
    prim[idx++] = face_rotation(z, 2);
  }
  // 12 row/column cycles for z in {0,1,2,3}
  for (std::uint8_t z = 0; z < 4; ++z) {
    prim[idx++] = row_cycle(z, true);   // rows cycle upwards
    prim[idx++] = row_cycle(z, false);  // rows cycle downwards
    prim[idx++] = column_cycle(z, true); // columns cycle rightwards
  }
  // 6 aggregate slice shifts (documented order)
  prim[idx++] = x_slice_shift(0);
  prim[idx++] = x_slice_shift(1);
  prim[idx++] = x_slice_shift(2);
  prim[idx++] = y_slice_shift(0);
  prim[idx++] = y_slice_shift(1);
  prim[idx++] = y_slice_shift(2);
  return prim;
}();

} // namespace

const std::array<Permutation, 36> &primitive_set() { return kPrimitives; }

} // namespace cube96
