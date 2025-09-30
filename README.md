<!-- SPDX-License-Identifier: MIT -->

# Cube96 Block Cipher

[![CI](https://github.com/OWNER/96bit-block-cipher/actions/workflows/ci.yml/badge.svg)](https://github.com/OWNER/96bit-block-cipher/actions/workflows/ci.yml)

Cube96 is a 96-bit block cipher implementation with both fast and constant-time
execution paths. The design operates on a 4×4×6 logical cube of bits and pairs
AES S-box substitution with key-dependent Rubik-style permutations derived from
SplitMix64. Keys and permutations are deterministically produced with an
HKDF(SHA-256) schedule.

> **Security disclaimer:** This cipher is experimental and not suitable for
> production deployments.

For technical details, round descriptions, and the full permutation catalogue
refer to [`docs/spec.md`](docs/spec.md).

## Features

- 96-bit block and key size with eight rounds plus post-whitening
- Two interchangeable implementations: table-driven fast path and bitsliced
  constant-time hardened path
- Compile-time selectable state layout. The default `zslice` layout stores two
  bytes per z-slice, while the optional `rowmajor` layout stores contiguous rows
  for improved cache behaviour on some platforms.
- HKDF-based key schedule with built-in SHA-256, HMAC, and SplitMix64 PRNG
- Deterministic per-round permutation generation from 36 documented primitives
- Static library (`libcube96`), CLI demo, throughput benchmark, and unit tests

## Building

Cube96 uses CMake and has no external dependencies.

```sh
mkdir -p build
cd build
cmake ..
cmake --build .
```

To explicitly select a layout mapping at configure time, pass
`-DCUBE96_LAYOUT=zslice` (default) or `-DCUBE96_LAYOUT=rowmajor` to the CMake
configure command. The chosen layout affects how `(x, y, z)` cube coordinates
map to bytes/bits in memory.

## Testing

Unit tests are registered with CTest. After building, run:

```sh
ctest
```

## Command-line Interface

The CLI encrypts or decrypts a single 96-bit block encoded as 24 hexadecimal
characters.

Every invocation prints a reminder that this is an experimental artifact:

> Research cipher — NOT FOR PRODUCTION. Key size chosen for tractability, not security.

```sh
./cube96_cli enc <hex-key-24> <hex-plaintext-24>
./cube96_cli dec <hex-key-24> <hex-ciphertext-24>
```

Example:

```sh
./cube96_cli enc 000102030405060708090a0b 0c0d0e0f1011121314151617
```

Exit codes:

- `0` – success
- `64` – incorrect CLI usage (missing/extra arguments)
- `65` – malformed key/plaintext/ciphertext hex input
- `66` – unknown mode (expected `enc` or `dec`)

## Reference Test Vectors

Deterministic known-answer tests for the default `zslice` layout are recorded in
[`vectors/cube96_kats.csv`](vectors/cube96_kats.csv). Regenerate them with the
`cube96_gen_kats` helper built alongside the CLI and benchmark:

```sh
./cube96_gen_kats vectors/cube96_kats.csv
```

Each entry specifies a 96-bit key, plaintext, and ciphertext in hexadecimal.
For example, `kat2_increment` can be reproduced with the CLI:

```sh
$ ./cube96_cli enc 000102030405060708090a0b 0c0d0e0f1011121314151617
Research cipher — NOT FOR PRODUCTION. Key size chosen for tractability, not security.
07e1e8afe74f1ffaf327549a
```

## Benchmark

The `cube96_bench` executable measures throughput for both implementations by
encrypting 64 MiB of random data in ECB mode. After building, run:

```sh
./cube96_bench
```

Set the `CUBE96_BENCH_BYTES` environment variable to reduce the workload during
CI runs or quick smoke tests (default is 64 MiB).

## Analysis Helpers

Lightweight tooling for exploratory cryptanalysis is provided under
[`analysis/`](analysis/).

- `python3 analysis/ddt_lat.py` computes the AES S-box difference distribution
  and linear approximation tables and writes CSV files next to the script.
- `python3 analysis/diff_trails.py --rounds 4 --branch 4` performs a simple
  branch-and-bound differential trail search (configurable depth/branching) and
  reports the best probability found.
- `python3 analysis/linear_bias.py --rounds 4 --samples 200000` estimates linear
  correlations by Monte Carlo sampling.

The scripts emit human-readable summaries and/or CSV outputs suitable for
further inspection in spreadsheets or plotting tools.

## Interoperability Notes

- Cube96 operates on 96-bit keys and blocks. All interfaces therefore expect
  exactly 24 hexadecimal characters per key or block with no `0x` prefix or
  separators.
- Hexadecimal input is interpreted in big-endian byte order (the first two hex
  characters correspond to the most significant byte). Upper- and lowercase hex
  digits are accepted.
- The CLI and provided vectors target the default `CUBE96_LAYOUT=zslice`
  configuration. Switching to the optional `rowmajor` layout preserves the
  ciphertexts but integrators who introduce additional compile-time changes
  should regenerate vectors to capture their configuration.

## Project Layout

- `include/` – public headers for the cipher, key schedule, permutation helpers,
  and constant-time utilities
- `src/` – library implementation files for the cipher core, key schedule,
  permutations, and S-box logic
- `tests/` – unit tests covering round-trips, known vectors, permutations,
  HKDF output, and avalanche behaviour
- `bench/` – throughput benchmark
- `tools/` – command-line demo
- `docs/` – supplementary documentation

## License

This project is available under the terms of the MIT License. See
[`LICENSE`](LICENSE).
