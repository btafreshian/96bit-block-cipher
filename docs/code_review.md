# Cube96 code review notes

## High-priority findings

- **Constant-time permutation helper still has key-dependent memory traffic.**
  `apply_permutation_ct` removes data-dependent branches, but the destination
  byte index varies with the permutation, which is itself derived from the
  secret key. Each round therefore performs writes to different stack offsets
  for different keys, leaking information over cache/bank side channels. A
  hardened implementation should keep the memory access pattern independent of
  the key, e.g. by accumulating all 96 destination bits in register masks per
  byte and storing them in a fixed order or by using a bitsliced permutation
  circuit that touches the same addresses every run.【F:src/perm.cpp†L60-L73】

- **SplitMix64 modulo reduction introduces observable bias.**
  Round permutation selection maps the 64-bit SplitMix output to 36 primitives
  via `% primitives.size()`. Because 2⁶⁴ is not a multiple of 36, the upper
  values occur slightly less often, creating a measurable bias after 12 draws
  per round. A rejection loop (`while (pick >= 36) pick = prng.next()`) would
  eliminate the bias and better respect the “curated basis” assumption in the
  spec.【F:src/cipher.cpp†L37-L47】【F:src/perm.cpp†L154-L188】

## Performance opportunities

- **Avoid redundant implementation checks inside the hot loops.**
  `encryptBlock` and `decryptBlock` branch on `impl_` twice per round. Hoisting a
  `const bool use_fast = impl_ == Impl::Fast;` and dispatching through function
  pointers or lambda captures would remove the repeated preprocessor-heavy
  `if`/`#if` blocks and make the round loop easier for the compiler to unroll.【F:src/cipher.cpp†L56-L138】

- **Eliminate extra round buffers.**
  Each round allocates a 12-byte `tmp` buffer and performs two `memcpy`
  operations. Swapping two `std::array<std::uint8_t, BlockBytes>` instances or
  alternating between two buffers avoids the allocations and copies, especially
  on decrypt where `tmp` is needed before SubBytes.【F:src/cipher.cpp†L75-L141】

- **Reuse HMAC pads during HKDF expand.**
  `hkdf_expand` reinitialises the inner/outer pads from scratch for every block,
  even though HKDF requires them only once. Carrying precomputed `ipad`/`opad`
  words (or exposing a light-weight `hmac_prenormalised` helper) would reduce
  the cost of key derivation and shrink stack usage in the hot loop.【F:src/key_schedule.cpp†L137-L225】

## Maintainability and documentation

- **Document the derived material layout explicitly.**
  The HKDF output is sliced into eight round keys, eight permutation seeds, and
  a whitening key. A short table or `static_assert` tying `okm` to
  `kRoundCount`/`kBlockBytes` would prevent silent layout drift if parameters
  change.【F:src/key_schedule.cpp†L242-L257】

- **Clarify hardened permutation guarantees in the README.**
  The README promises a constant-time hardened path but does not warn that only
  the S-box is bitsliced. Mentioning the remaining key-dependent permutation
  writes (or linking to a rationale) would set expectations for users evaluating
  the hardened mode.【F:README.md†L19-L76】

- **Provide user-facing guidance on selecting `Impl::Hardened`.**
  The quick-start example instantiates `CubeCipher` without showing how to force
  the hardened path at runtime. Adding a snippet that passes `Impl::Hardened` or
  checks `CubeCipher::hasFastImpl()` would help integrators who want constant
  time regardless of build flags.【F:README.md†L38-L58】【F:include/cube96/cipher.hpp†L12-L47】

