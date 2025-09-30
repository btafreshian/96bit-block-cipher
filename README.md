# Cube96 Block Cipher

**Research cipher — NOT FOR PRODUCTION. Key size chosen for tractability, not security.**

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

For experimenting with stream-like usage, wrap Cube96 in CTR mode by combining a
96-bit nonce with a monotonically increasing block counter. Never reuse a
nonce+counter pair under the same key, and bound messages so the 32-bit counter
does not wrap.

## Why 96-bit?

Cube96 was designed as a tractable target for differential and linear analysis
exercises rather than a production-strength primitive. The compact state keeps
brute-force and meet-in-the-middle explorations within reach for classroom and
capture-the-flag scenarios while still showcasing modern design techniques.

## Reference Test Vectors

Deterministic known-answer tests (KATs) for the default `zslice` layout build
are published under [`vectors/`](vectors/). Each CSV row lists the hexadecimal
key, plaintext, and ciphertext for a single block encryption. Changing the
layout at compile time (e.g. configuring CMake with
`-DCUBE96_LAYOUT=rowmajor`) produces a different state mapping and therefore a
different set of vectors.

For example, the all-zero key/plaintext vector encrypts to:

```
key        = 000000000000000000000000
plaintext  = 000000000000000000000000
ciphertext = b6393ae0d2e9a2c771e619fa
```

You can verify the CSV entry with the CLI:

```sh
./cube96_cli enc 000000000000000000000000 000000000000000000000000
# prints b6393ae0d2e9a2c771e619fa
```

## Interoperability Notes

- Cube96 uses fixed 96-bit keys and 96-bit blocks (12 bytes each). Inputs must
  be encoded as exactly 24 hexadecimal characters.
- Hexadecimal strings are interpreted big-endian: the first two characters map
  to the first byte fed into the cipher, and so on. The CLI accepts both upper-
  and lower-case hex digits and emits lower-case output.
- Internal state bits are packed most-significant-bit first within each byte.
  Selecting the optional `rowmajor` layout only affects the in-memory mapping,
  not the interpretation of external hex inputs.

Exit codes:

- `0` – success
- `64` – incorrect CLI usage (missing/extra arguments)
- `65` – malformed key/plaintext/ciphertext hex input
- `66` – unknown mode (expected `enc` or `dec`)

## Benchmark

The `cube96_bench` executable measures throughput for both implementations by
encrypting 64 MiB of random data in ECB mode. After building, run:

```sh
./cube96_bench
```

Set the `CUBE96_BENCH_BYTES` environment variable to reduce the workload during
CI runs or quick smoke tests (default is 64 MiB). The benchmark uses ECB mode
solely to measure raw throughput; it lacks semantic security and must not be
applied to real-world data.

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

For a step-by-step walkthrough with sample outputs, see the
[`docs/analysis_guide.md`](docs/analysis_guide.md) companion guide.

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
