#include "src/siphash.h"
#include <cstdint>
#include <cstddef>

namespace cuckoo_sip {

static inline uint64_t ROTL64(uint64_t x, unsigned b) {
  return (x << b) | (x >> (64 - b));
}

static inline void sipround(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3) {
  v0 += v1; v2 += v3; v1 = ROTL64(v1, 13); v3 = ROTL64(v3, 16);
  v1 ^= v0; v3 ^= v2; v0 = ROTL64(v0, 32);
  v2 += v1; v0 += v3; v1 = ROTL64(v1, 17); v3 = ROTL64(v3, 21);
  v1 ^= v2; v3 ^= v0; v2 = ROTL64(v2, 32);
}

uint64_t siphash_u64_rounds(const Key128& key, uint64_t nonce, int c, int d) {
  uint64_t v0 = 0x736f6d6570736575ULL ^ key.k0;
  uint64_t v1 = 0x646f72616e646f6dULL ^ key.k1;
  uint64_t v2 = 0x6c7967656e657261ULL ^ key.k0;
  uint64_t v3 = 0x7465646279746573ULL ^ key.k1;

  uint64_t m = nonce;
  uint64_t b = (uint64_t)8ULL << 56; // msg length in high byte
  b |= m;

  v3 ^= b;
  for (int i = 0; i < c; ++i) sipround(v0, v1, v2, v3);
  v0 ^= b;

  v2 ^= 0xff;
  for (int i = 0; i < d; ++i) sipround(v0, v1, v2, v3);

  return v0 ^ v1 ^ v2 ^ v3;
}

void siphash12_batch(const Key128& key, const uint64_t* nonces, uint64_t* out, size_t count) {
  for (size_t i = 0; i < count; ++i) out[i] = siphash_u64_rounds(key, nonces[i], 1, 2);
}

void siphash24_batch(const Key128& key, const uint64_t* nonces, uint64_t* out, size_t count) {
  for (size_t i = 0; i < count; ++i) out[i] = siphash_u64_rounds(key, nonces[i], 2, 4);
}

} // namespace cuckoo_sip
