# Cube96 Block Cipher

Cube96 is a 96-bit block cipher implementation with both fast and constant-time
execution paths. The design operates on a 4×4×6 logical cube of bits and pairs
AES S-box substitution with key-dependent Rubik-style permutations derived from
SplitMix64. Keys and permutations are deterministically produced with an
HKDF(SHA-256) schedule.

## Features

- 96-bit block and key size with eight rounds plus post-whitening
- Two interchangeable implementations: table-driven fast path and bitsliced
  constant-time hardened path
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

## Testing

Unit tests are registered with CTest. After building, run:

```sh
ctest
```

## Command-line Interface

The CLI encrypts or decrypts a single 96-bit block encoded as 24 hexadecimal
characters.

```sh
./cube96_cli enc <hex-key-24> <hex-plaintext-24>
./cube96_cli dec <hex-key-24> <hex-ciphertext-24>
```

Example:

```sh
./cube96_cli enc 000102030405060708090a0b 0c0d0e0f1011121314151617
```

## Benchmark

The `cube96_bench` executable measures throughput for both implementations by
encrypting 64 MiB of random data in ECB mode. After building, run:

```sh
./cube96_bench
```

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
