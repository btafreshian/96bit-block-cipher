#include "cube96/key_schedule.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "cube96/endian.hpp"

namespace cube96 {
namespace {

struct Sha256Ctx {
  std::uint32_t h[8];
  std::uint64_t bit_len;
  std::size_t buffer_len;
  std::uint8_t buffer[64];
};

const std::uint32_t kSha256Init[8] = {
    0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
    0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u};

const std::uint32_t kSha256K[64] = {
    0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u, 0x3956C25Bu,
    0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u, 0xD807AA98u, 0x12835B01u,
    0x243185BEu, 0x550C7DC3u, 0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u,
    0xC19BF174u, 0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu,
    0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu, 0x983E5152u,
    0xA831C66Du, 0xB00327C8u, 0xBF597FC7u, 0xC6E00BF3u, 0xD5A79147u,
    0x06CA6351u, 0x14292967u, 0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu,
    0x53380D13u, 0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
    0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u, 0xD192E819u,
    0xD6990624u, 0xF40E3585u, 0x106AA070u, 0x19A4C116u, 0x1E376C08u,
    0x2748774Cu, 0x34B0BCB5u, 0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu,
    0x682E6FF3u, 0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u,
    0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u};

inline std::uint32_t rotr(std::uint32_t x, std::uint32_t r) {
  return (x >> r) | (x << (32 - r));
}

void sha256_init(Sha256Ctx &ctx) {
  std::copy(std::begin(kSha256Init), std::end(kSha256Init), ctx.h);
  ctx.bit_len = 0;
  ctx.buffer_len = 0;
}

void sha256_compress(Sha256Ctx &ctx, const std::uint8_t block[64]) {
  std::uint32_t w[64];
  for (int i = 0; i < 16; ++i) {
    w[i] = load_be32(block + 4 * i);
  }
  for (int i = 16; i < 64; ++i) {
    std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }

  std::uint32_t a = ctx.h[0];
  std::uint32_t b = ctx.h[1];
  std::uint32_t c = ctx.h[2];
  std::uint32_t d = ctx.h[3];
  std::uint32_t e = ctx.h[4];
  std::uint32_t f = ctx.h[5];
  std::uint32_t g = ctx.h[6];
  std::uint32_t h = ctx.h[7];

  for (int i = 0; i < 64; ++i) {
    std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    std::uint32_t ch = (e & f) ^ ((~e) & g);
    std::uint32_t temp1 = h + S1 + ch + kSha256K[i] + w[i];
    std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    std::uint32_t temp2 = S0 + maj;

    h = g;
    g = f;
    f = e;
    e = d + temp1;
    d = c;
    c = b;
    b = a;
    a = temp1 + temp2;
  }

  ctx.h[0] += a;
  ctx.h[1] += b;
  ctx.h[2] += c;
  ctx.h[3] += d;
  ctx.h[4] += e;
  ctx.h[5] += f;
  ctx.h[6] += g;
  ctx.h[7] += h;
}

void sha256_update(Sha256Ctx &ctx, const std::uint8_t *data, std::size_t len) {
  ctx.bit_len += static_cast<std::uint64_t>(len) * 8u;
  while (len > 0) {
    std::size_t take = std::min<std::size_t>(len, 64 - ctx.buffer_len);
    std::memcpy(ctx.buffer + ctx.buffer_len, data, take);
    ctx.buffer_len += take;
    data += take;
    len -= take;
    if (ctx.buffer_len == 64) {
      sha256_compress(ctx, ctx.buffer);
      ctx.buffer_len = 0;
    }
  }
}

void sha256_final(Sha256Ctx &ctx, std::uint8_t out[32]) {
  ctx.buffer[ctx.buffer_len++] = 0x80;
  if (ctx.buffer_len > 56) {
    while (ctx.buffer_len < 64) {
      ctx.buffer[ctx.buffer_len++] = 0x00;
    }
    sha256_compress(ctx, ctx.buffer);
    ctx.buffer_len = 0;
  }
  while (ctx.buffer_len < 56) {
    ctx.buffer[ctx.buffer_len++] = 0x00;
  }
  std::uint8_t len_be[8];
  store_be64(ctx.bit_len, len_be);
  std::memcpy(ctx.buffer + 56, len_be, 8);
  sha256_compress(ctx, ctx.buffer);
  for (int i = 0; i < 8; ++i) {
    store_be32(ctx.h[i], out + 4 * i);
  }
}

struct HmacSha256Ctx {
  Sha256Ctx inner;
  Sha256Ctx outer;
};

void hmac_init(HmacSha256Ctx &ctx, const std::uint8_t *key, std::size_t key_len) {
  std::uint8_t key_block[64];
  if (key_len > 64) {
    Sha256Ctx hash_ctx;
    sha256_init(hash_ctx);
    sha256_update(hash_ctx, key, key_len);
    std::uint8_t hashed[32];
    sha256_final(hash_ctx, hashed);
    std::memset(key_block, 0, sizeof(key_block));
    std::memcpy(key_block, hashed, 32);
  } else {
    std::memset(key_block, 0, sizeof(key_block));
    std::memcpy(key_block, key, key_len);
  }

  std::uint8_t ipad[64];
  std::uint8_t opad[64];
  for (int i = 0; i < 64; ++i) {
    ipad[i] = static_cast<std::uint8_t>(key_block[i] ^ 0x36u);
    opad[i] = static_cast<std::uint8_t>(key_block[i] ^ 0x5Cu);
  }

  sha256_init(ctx.inner);
  sha256_update(ctx.inner, ipad, 64);
  sha256_init(ctx.outer);
  sha256_update(ctx.outer, opad, 64);
}

void hmac_update(HmacSha256Ctx &ctx, const std::uint8_t *data, std::size_t len) {
  sha256_update(ctx.inner, data, len);
}

void hmac_final(HmacSha256Ctx &ctx, std::uint8_t out[32]) {
  std::uint8_t inner_digest[32];
  sha256_final(ctx.inner, inner_digest);
  sha256_update(ctx.outer, inner_digest, 32);
  sha256_final(ctx.outer, out);
}

} // namespace

Sha256Digest sha256(const std::uint8_t *data, std::size_t len) {
  Sha256Ctx ctx;
  sha256_init(ctx);
  sha256_update(ctx, data, len);
  std::uint8_t out[32];
  sha256_final(ctx, out);
  Sha256Digest digest{};
  for (int i = 0; i < 8; ++i) {
    digest.h[i] = load_be32(out + 4 * i);
  }
  return digest;
}

void hmac_sha256(const std::uint8_t *key, std::size_t key_len,
                 const std::uint8_t *data, std::size_t data_len,
                 std::uint8_t out[32]) {
  HmacSha256Ctx ctx;
  hmac_init(ctx, key, key_len);
  hmac_update(ctx, data, data_len);
  hmac_final(ctx, out);
}

void hkdf_expand(const std::uint8_t prk[32], const std::uint8_t *info,
                 std::size_t info_len, std::uint8_t *out, std::size_t out_len) {
  std::uint8_t previous[32];
  std::size_t prev_len = 0;
  std::size_t generated = 0;
  std::uint8_t counter = 1;

  HmacSha256Ctx base_ctx;
  hmac_init(base_ctx, prk, 32);

  while (generated < out_len) {
    HmacSha256Ctx ctx = base_ctx;
    if (prev_len > 0) {
      hmac_update(ctx, previous, prev_len);
    }
    if (info_len > 0) {
      hmac_update(ctx, info, info_len);
    }
    hmac_update(ctx, &counter, 1);
    std::uint8_t block[32];
    hmac_final(ctx, block);
    std::size_t to_copy = std::min<std::size_t>(out_len - generated, 32);
    std::memcpy(out + generated, block, to_copy);
    std::memcpy(previous, block, 32);
    prev_len = 32;
    generated += to_copy;
    ++counter;
  }
}

DerivedMaterial derive_material(const std::uint8_t key[kKeyBytes]) {
  // HKDF salt encodes ASCII "StagedCube's-96-HKDF-V1" padded to 32 bytes with
  // zeros. The fixed salt and info string guarantee deterministic derivation
  // across platforms.
  static const std::uint8_t kSalt[32] = {
      0x53, 0x74, 0x61, 0x67, 0x65, 0x64, 0x43, 0x75,
      0x62, 0x65, 0x27, 0x73, 0x2D, 0x39, 0x36, 0x2D,
      0x48, 0x4B, 0x44, 0x46, 0x2D, 0x56, 0x31, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  static const std::uint8_t kInfo[] = "Cube96-RK-PS-Post-v1"; // ASCII, no NUL

  std::uint8_t prk[32];
  hmac_sha256(kSalt, sizeof(kSalt), key, kKeyBytes, prk);

  std::uint8_t okm[172];
  hkdf_expand(prk, kInfo, sizeof(kInfo) - 1, okm, sizeof(okm));

  static_assert(kRoundCount * kBlockBytes + kRoundCount * 8 + kBlockBytes == sizeof(okm),
                "HKDF layout must match derived material footprint");

  DerivedMaterial material;
  std::size_t offset = 0;
  // okm layout: round keys (kRoundCount × kBlockBytes), permutation seeds
  // (kRoundCount × 8), and post-whitening block (kBlockBytes).
  for (std::size_t r = 0; r < kRoundCount; ++r) {
    std::copy_n(okm + offset, kBlockBytes, material.round_keys[r].begin());
    offset += kBlockBytes;
  }
  for (std::size_t r = 0; r < kRoundCount; ++r) {
    std::copy_n(okm + offset, 8, material.perm_seeds[r].begin());
    offset += 8;
  }
  std::copy_n(okm + offset, kBlockBytes, material.post_whitening.begin());

  return material;
}

} // namespace cube96
