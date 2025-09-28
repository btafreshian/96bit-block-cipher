# Cube96 Block Cipher Implementation Notes

This repository implements the Cube96 96-bit block cipher exactly as outlined in
the project brief. The cipher operates on a 4×4×6 logical cube of bits using
AES S-boxes, key-dependent permutations composed from Rubik-style primitives,
and an HKDF(SHA-256) key schedule.

## State Layouts and Bit Indexing

A Cube96 block contains 96 bits arranged as coordinates (x, y, z) with
x ∈ [0,3], y ∈ [0,3], and z ∈ [0,5]. Two memory layouts are supported:

- **Z-slice layout (default)** — slices are ordered by z. Inside a slice the
  16 bits follow row-major (y, then x) order with MSB-first packing within each
  byte. The helper functions implement:

  ```text
  idx_of(x,y,z)  = 16·z + 4·y + x
  byte_index     = 2·z + ⌊(4·y + x)/8⌋
  bit_in_byte    = 7 − ((4·y + x) mod 8)
  ```

- **Row-major layout** — enabled with `-DCUBE96_LAYOUT=rowmajor`. Bits are
  ordered by row first, then by slice, then by column:

  ```text
  idx_of(x,y,z)  = ((6·y) + z) · 4 + x
  byte_index     = ⌊idx_of/8⌋
  bit_in_byte    = 7 − (idx_of mod 8)
  ```

Both layouts share the same `idx_of`, `xyz_of`, `get_bit`, and `set_bit`
helpers. Layout selection happens at build time through the
`CUBE96_LAYOUT` CMake cache variable (`zslice` by default). Unit tests verify
that `xyz_of(idx_of(...))` is always identity for the active mapping.

### Worked Primitive Example

Consider a block where only the bit at (x=1, y=2, z=0) is set. With the default
z-slice layout the bit index is `16·0 + 4·2 + 1 = 9`, which lives in byte 1 at
bit position 6. Applying the “row-up” primitive on layer z=0 transforms
(x, y, z) → (x, (y+1) mod 4, z); the bit therefore moves to (1, 3, 0). In
z-slice layout this becomes index 13 (byte 1, bit position 2), illustrating how
primitives permute individual bits without altering byte packing logic.

## Permutation Primitive Catalogue

Per-round permutations are composed from 36 bijective primitives. IDs are
stable and documented below. Each mapping acts on the cube coordinates while
leaving unaffected coordinates unchanged.

### Face rotations (IDs 0–17)

For each z ∈ {0,…,5}:

| ID range | Primitive                 | Mapping                               |
|---------:|---------------------------|----------------------------------------|
| 3·z + 0  | Face z=z 90° clockwise    | (x, y, z) → (3 − y, x, z)              |
| 3·z + 1  | Face z=z 90° counter-CW   | (x, y, z) → (y, 3 − x, z)              |
| 3·z + 2  | Face z=z 180°             | (x, y, z) → (3 − x, 3 − y, z)          |

### Row/column cycles (IDs 18–29)

For each z ∈ {0,1,2,3} the following primitives are included:

| ID range | Primitive            | Mapping                                      |
|---------:|----------------------|-----------------------------------------------|
| 12 + 3·z | Row cycle up         | (x, y, z) → (x, (y + 1) mod 4, z)             |
| 12 + 3·z + 1 | Row cycle down   | (x, y, z) → (x, (y + 3) mod 4, z)             |
| 12 + 3·z + 2 | Column cycle right | (x, y, z) → ((x + 1) mod 4, y, z)           |

Column-left cycles are omitted because applying the right cycle thrice yields
an equivalent transformation, keeping the curated set compact.

### Aggregate z-shifts (IDs 30–35)

| ID | Primitive                  | Mapping                                      |
|----|----------------------------|-----------------------------------------------|
| 30 | X-slice x=0 shift +1       | (x, y, z) → (x, y, (z + 1) mod 6) for x=0     |
| 31 | X-slice x=1 shift +1       | Same as above for x=1                         |
| 32 | X-slice x=2 shift +1       | Same as above for x=2                         |
| 33 | Y-slice y=0 shift +1       | (x, y, z) → (x, y, (z + 1) mod 6) for y=0     |
| 34 | Y-slice y=1 shift +1       | Same as above for y=1                         |
| 35 | Y-slice y=2 shift +1       | Same as above for y=2                         |

All primitives are bijections; each is a permutation of the 96 cube positions.
Compositions inherit bijectivity, so the resulting per-round permutations are
also bijective. `perm.cpp` lists the mapping formulas verbatim alongside the ID
layout for reproducibility.

### Round Composition

During `CubeCipher::setKey`, SplitMix64 seeded from HKDF output selects 12
primitive indices per round. Composition is left-to-right: the first selected
primitive is applied first, then the second, and so on. Decryption uses stored
inverse permutations computed through `invert`, guaranteeing reversible rounds.

## HKDF Parameters

Key derivation follows HKDF with SHA-256 using the fixed constants below:

- **Salt (32 bytes)** — ASCII `"StagedCube's-96-HKDF-V1"` padded with zeros:
  `53 74 61 67 65 64 43 75 62 65 27 73 2D 39 36 2D 48 4B 44 46 2D 56 31 00 00 00 00 00 00 00 00`.
- **Info string** — ASCII `"Cube96-RK-PS-Post-v1"`.
- HKDF expand length: 172 bytes producing 8 round keys (96-bit), 8 permutation
  seeds (64-bit), and one 96-bit post-whitening key, in that order.

Unit tests include a fixed vector for the all-zero master key to ensure that
implementations on different platforms derive identical material.

## Cryptanalysis Helpers

Three analysis utilities live under `analysis/` and build as part of the CMake
project:

- `cube96_ddt_lat` computes the AES S-box DDT and LAT, writing CSV matrices.
  It reports a maximum differential uniformity of 4 and a maximum absolute
  linear bias of 32/256, matching Rijndael properties.
- `cube96_diff_trails` performs a branch-and-bound search for high-probability
  differential trails up to four rounds. With the zero key and `--branch 2` the
  best trail has weight ≈ 27.0 (probability ≈ 2⁻²⁷).
- `cube96_linear_bias` estimates linear correlation for partial rounds via
  Monte Carlo sampling. Using the default masks over four rounds and 65,536
  samples yields an observed correlation of about −9.5×10⁻⁴ (bias ≈ −4.7×10⁻⁴).

See the project README for invocation examples. Generated CSV files and console
summaries provide starting points for further cryptanalysis.

## Continuous Integration

The GitHub Actions workflow (`.github/workflows/ci.yml`) builds on Linux,
macOS, and Windows, runs the unit test suite, and exercises the throughput
benchmarks to ensure both fast and hardened implementations remain healthy.

## Licensing

All source files are distributed under the MIT License (see `LICENSE`).
