#include "src/siphash.h"
#include <cstdint>
#include <cstring>

namespace siphash {

static inline uint64_t rotl64(uint64_t x, int b) {
  return (x << b) | (x >> (64 - b));
}

static inline void sipround(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3) {
  v0 += v1; v1 = rotl64(v1, 13); v1 ^= v0; v0 = rotl64(v0, 32);
  v2 += v3; v3 = rotl64(v3, 16); v3 ^= v2;
  v0 += v3; v3 = rotl64(v3, 21); v3 ^= v0;
  v2 += v1; v1 = rotl64(v1, 17); v1 ^= v2; v2 = rotl64(v2, 32);
}

uint64_t siphash_c_d_u64(const Key& key, uint64_t m, int c, int d) {
  uint64_t v0 = 0x736f6d6570736575ULL ^ key.k0;
  uint64_t v1 = 0x646f72616e646f6dULL ^ key.k1;
  uint64_t v2 = 0x6c7967656e657261ULL ^ key.k0;
  uint64_t v3 = 0x7465646279746573ULL ^ key.k1;

  const uint64_t b = (uint64_t)8 << 56; // length=8

  v3 ^= m;
  for (int i = 0; i < c; ++i) sipround(v0, v1, v2, v3);
  v0 ^= m;

  v3 ^= b;
  for (int i = 0; i < c; ++i) sipround(v0, v1, v2, v3);
  v0 ^= b;

  v2 ^= 0xff;
  for (int i = 0; i < d; ++i) sipround(v0, v1, v2, v3);

  return v0 ^ v1 ^ v2 ^ v3;
}

void siphash12_batch_u64(const Key& key, const uint64_t* in, uint64_t* out, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    out[i] = siphash_c_d_u64(key, in[i], 1, 2);
  }
}

void siphash24_batch_u64(const Key& key, const uint64_t* in, uint64_t* out, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    out[i] = siphash_c_d_u64(key, in[i], 2, 4);
  }
}

} // namespace siphash
