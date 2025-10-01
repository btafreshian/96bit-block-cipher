# Cube96 Block Cipher

> ⚠️ **Research cipher — NOT FOR PRODUCTION.** The 96-bit parameters keep the
> state space tractable for exploration and are not intended to meet modern
> security targets. ECB mode usage is limited to performance measurement; any
> CTR experiments must use unique nonces and stay well below 2³² blocks of data.

Cube96 is a 96-bit block cipher implementation with both fast and constant-time
execution paths. The design operates on a 4×4×6 logical cube of bits and pairs
AES S-box substitution with key-dependent Rubik-style permutations derived from
SplitMix64. Keys and permutations are deterministically produced with an
HKDF(SHA-256) schedule. Hardened mode swaps the table S-box for a bitsliced
variant and keeps the permutation memory access pattern fixed across keys; it
still shares the same algebraic structure and is not a proof of constant-time
behaviour in the cryptographic sense.

For technical details, round descriptions, and the full permutation catalogue
refer to [`docs/spec.md`](docs/spec.md).

## Features

- 96-bit block and key size with eight rounds plus post-whitening
- Two interchangeable implementations: table-driven fast path and bitsliced
  constant-time hardened path, selectable at runtime with build-time policy
- Compile-time selectable state layout. The default `zslice` layout stores two
  bytes per z-slice, while the optional `rowmajor` layout stores contiguous rows
  for improved cache behaviour on some platforms.
- HKDF-based key schedule with built-in SHA-256, HMAC, and SplitMix64 PRNG
- Deterministic per-round permutation generation from 36 documented primitives
- Installable static library (`libcube96`), CLI demo, throughput benchmark, and
  unit tests with CTest labels (`KAT`, `PERM`, `HKDF`, `CT`, `CLI`)

### Why 96-bit?

The cipher’s state maps neatly onto a 4×4×6 cube, which keeps the permutation
catalogue manageable for research, teaching, and tooling demonstrations. The
short block and key sizes are not meant to provide production-grade security.

## Quick start (C++)

```cpp
#include <array>
#include "cube96/cipher.hpp"

int main() {
  std::array<std::uint8_t, cube96::CubeCipher::KeyBytes> key{{
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}};
  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> block{{
      0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17}};

  cube96::CubeCipher cipher;  // Defaults to the fast path unless forced hardening.
  cipher.setKey(key.data());

  std::array<std::uint8_t, cube96::CubeCipher::BlockBytes> out{};
  cipher.encryptBlock(block.data(), out.data());

  return static_cast<int>(out[0]);  // Prevent optimisation away.
}
```

## Building

Cube96 uses portable CMake and has no external dependencies.

```sh
cmake -S . -B build
cmake --build build
```

Useful configuration options:

| Option | Default | Effect |
| --- | --- | --- |
| `-DCUBE96_LAYOUT={zslice,rowmajor}` | `zslice` | Selects the state bit layout. |
| `-DCUBE96_FORCE_CONSTANT_TIME=ON` | `OFF` | Forces the hardened implementation and removes table lookups. |
| `-DCUBE96_ENABLE_FAST_IMPL=OFF` | `ON` | (Implicitly set when forcing constant-time) disables the fast S-box tables. |

Install the library and headers into a prefix:

```sh
cmake --install build --prefix /tmp/cube96-install
```

Consumers can then use the generated package configuration:

```cmake
find_package(cube96 CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE cube96::cube96)
```

## Testing

Unit tests are registered with CTest and carry labels for selective execution.

```sh
ctest --test-dir build --output-on-failure
ctest --test-dir build -L KAT --output-on-failure  # only known-answer tests
```

The long-running `test_roundtrip` binary now defaults to 10,000 random
round-trips to keep CI runs under the workflow timeout. Increase coverage
locally by exporting `CUBE96_TEST_ITERATIONS=<count>` before invoking CTest.

## Command-line Interface

The CLI encrypts or decrypts a single 96-bit block encoded as 24 hexadecimal
characters.

Every invocation prints a reminder that this is an experimental artifact:

> Research cipher — NOT FOR PRODUCTION. Key size chosen for tractability, not security.

```sh
./cube96_cli enc <hex-key-24> <hex-plaintext-24>
./cube96_cli dec <hex-key-24> <hex-ciphertext-24>
```

Example round-trip using the first vector from `vectors/cube96_kats_zslice.csv`:

```sh
./cube96_cli enc 000000000000000000000000 000000000000000000000000
# stderr: Research cipher warning
# stdout: b6393ae0d2e9a2c771e619fa
./cube96_cli dec 000000000000000000000000 b6393ae0d2e9a2c771e619fa
# stdout: 000000000000000000000000
```

Invalid keys or blocks (wrong length or non-hex characters) trigger exit code
`65` with a descriptive error. An unknown mode returns `66`, and supplying the
wrong number of arguments returns `64` after printing usage.

## Reference Test Vectors

Deterministic known-answer tests (KATs) for both layouts are published under
[`vectors/`](vectors/). Each CSV row lists the hexadecimal key, plaintext, and
ciphertext for a single block encryption.

- `cube96_kats_zslice.csv` – default layout (matches the CLI example above)
- `cube96_kats_rowmajor.csv` – layout selected via `-DCUBE96_LAYOUT=rowmajor`

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
CI runs or quick smoke tests (default is 64 MiB). The value must be a positive
multiple of 12 bytes (one block); invalid overrides cause the benchmark to exit
with a non-zero status.

## Sanity run

Copy/paste the following block for a quick verification of the default
configuration, labelled test subsets, CLI behaviour, and the benchmark smoke
test:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -L KAT --output-on-failure
./build/cube96_cli enc 000000000000000000000000 000000000000000000000000
CUBE96_BENCH_BYTES=120 ./build/cube96_bench
```

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
### Selecting the hardened implementation

Runtime callers can force the constant-time pathway regardless of build flags
by instantiating the cipher with `Impl::Hardened`. The helpers
`CubeCipher::hasFastImpl()` and `CubeCipher::DefaultImpl` expose the build
policy so applications can adapt when the fast implementation is disabled.

```cpp
auto impl = cube96::CubeCipher::hasFastImpl()
                ? cube96::CubeCipher::Impl::Hardened
                : cube96::CubeCipher::DefaultImpl;
cube96::CubeCipher hardened_cipher(impl);
```

`Impl::Hardened` removes lookup tables and uses a constant-memory permutation
routine; it does not eliminate timing variation from unrelated platform
effects, so consumers should still perform their own side-channel analysis when
considering integration.

