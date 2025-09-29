# Cube96 Block Cipher Implementation Notes

This repository contains an implementation of the Cube96 96-bit block cipher.
The core design follows the requirements from the project brief:

- 96-bit block and key size, organised as a 4×4×6 logical cube.
- Eight rounds consisting of AddRoundKey → SubBytes → key-dependent
  permutation, followed by a 96-bit post-whitening key.
- AES S-box for substitution. The fast implementation uses table lookups,
  while the hardened implementation computes the S-box via constant-time GF
  arithmetic with no data-dependent memory access.
- Per-round permutations are assembled from a curated set of 36 Rubik-style
  primitives driven by SplitMix64 seeded from the HKDF output.
- The key schedule uses HKDF with SHA-256 and fixed salt/info parameters to
  derive eight 96-bit round keys, eight 64-bit permutation seeds, and the
  post-whitening key.

Deliverables include the `libcube96` static library, unit tests, a throughput
benchmark, cryptanalysis helpers, and a command-line demonstration tool.

## State Layouts

Cube96 operates on a logical `(x, y, z)` cube with dimensions `4 × 4 × 6`. Two
memory layouts are provided at compile time via the `CUBE96_LAYOUT` CMake
option.

### `zslice` layout (default)

- Bit index: `idx = 16·z + 4·y + x`
- Coordinate recovery: `z = ⌊idx / 16⌋`, `in_slice = idx mod 16`, `y = ⌊in_slice / 4⌋`,
  `x = in_slice mod 4`
- Byte index: each z-slice stores two bytes (16 bits) ordered by rows, so the
  byte index is `2·z + ⌊(idx mod 16) / 8⌋`
- Bit ordering: bits are stored MSB-first inside each byte with offset
  `7 − ((idx mod 16) mod 8)`

### `rowmajor` layout

- Bit index: `idx = 24·y + 6·x + z`
- Coordinate recovery: `y = ⌊idx / 24⌋`, `in_row = idx mod 24`,
  `x = ⌊in_row / 6⌋`, `z = in_row mod 6`
- Byte index: each row (fixed `y`) stores three bytes (24 bits) laid out with
  x as the major coordinate, so the byte index is `3·y + ⌊(idx mod 24) / 8⌋`
- Bit ordering: bits remain MSB-first inside each byte with offset
  `7 − (idx mod 8)`

Both layouts are bijective mappings between coordinates and bit indices. The
`zslice` option matches the original specification, while `rowmajor` improves
locality when iterating by rows and is selected via `-DCUBE96_LAYOUT=rowmajor`.

## Permutation Primitive Catalogue

Each round permutation is assembled from the following 36 bijective primitives
defined on the cube coordinates. `R(v, k)` denotes rotation/shift modulo `k`.

### Face rotations (`indices 0–17`)

For every `z ∈ {0,…,5}`:

- `F_z^CW` : `(x, y, z) → (3 − y, x, z)`
- `F_z^CCW`: `(x, y, z) → (y, 3 − x, z)`
- `F_z^180`: `(x, y, z) → (3 − x, 3 − y, z)`

### Row/column cycles (`indices 18–29`)

For `z ∈ {0,…,3}`:

- `Row_z^↑`: `(x, y, z) → (x, R(y + 1, 4), z)`
- `Row_z^↓`: `(x, y, z) → (x, R(y + 3, 4), z)`
- `Col_z^→`: `(x, y, z) → (R(x + 1, 4), y, z)`

### Aggregate slice shifts (`indices 30–35`)

- `X_{x0}` for `x0 ∈ {0,1,2}`: coordinates with `x = x0` move to
  `(x0, y, R(z + 1, 6))`; all other coordinates are unchanged.
- `Y_{y0}` for `y0 ∈ {0,1,2}`: coordinates with `y = y0` move to
  `(x, y0, R(z + 1, 6))`; all other coordinates are unchanged.

All primitives are bijective because each mapping performs coordinate
permutations with modular addition; every output coordinate triple uniquely
determines its input. The inverse of any primitive is obtained by reversing the
rotation direction or subtracting the modular shift.

### Composition per round

The HKDF stage provides eight 64-bit seeds. For round `r`, the seed is expanded
with SplitMix64 to produce 12 picks from the primitive set. Starting from the
identity permutation, the selected primitives are composed in order to form the
round permutation `P_r`. The decrypt path uses `P_r^{-1}`, computed explicitly
by inverting the final permutation.

### Worked example

Consider the default `zslice` layout and the primitive `F_0^CW` (clockwise
rotation of the `z = 0` face). Start with a block whose most significant bit is
set only at coordinate `(x, y, z) = (0, 0, 0)`, which corresponds to the first
byte holding value `0x80`. Applying `F_0^CW` maps `(0, 0, 0)` to `(3, 0, 0)`,
which becomes bit index `3` (bit mask `0x10` in the first byte). The resulting
block therefore has its first byte equal to `0x10`, demonstrating the 90°
rotation across the face. Any other bit in the `z = 0` slice follows the same
row/column rotation, preserving Hamming weight.

## HKDF Parameters

The key schedule uses HKDF with SHA-256. The following constants are compiled
in and documented to guarantee deterministic material across platforms:

- Salt (`32` bytes): ASCII string `"StagedCube's-96-HKDF-V1"` padded with zeros
  to 32 bytes.
- Info: ASCII string `"Cube96-RK-PS-Post-v1"` (no terminating NUL).

HKDF output (`172` bytes) is partitioned as eight round keys, eight 64-bit
permutation seeds (big-endian), and the final post-whitening key.

## Cryptanalysis Helpers

The `analysis/` directory provides scripts to aid exploratory cryptanalysis:

- `ddt_lat.py` generates CSV files for the AES S-box difference distribution
  table (DDT) and linear approximation table (LAT).
- `diff_trails.py` performs a simple branch-and-bound search for the best
  differential trails up to four rounds. With the default parameters the best
  probability observed is approximately `2^{-27}` after four rounds (weight 27).
- `linear_bias.py` estimates linear correlations via Monte Carlo sampling. The
  strongest absolute bias observed for four rounds with 200k samples was about
  `2^{-8.6}`.

These figures are not a substitute for exhaustive analysis but provide sanity
checks against trivial weaknesses and match the outputs recorded by the helper
scripts.
