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
- The key schedule uses HKDF with SHA-256 and a fixed salt/info string to
  derive eight 96-bit round keys, eight 64-bit permutation seeds, and the
  post-whitening key.

Deliverables include the `libcube96` static library, unit tests, a throughput
benchmark, and a command-line demonstration tool.
