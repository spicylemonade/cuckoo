#include "siphash12.h"

#include <cstdint>
#include <cstddef>

namespace cuckoo_sip {

static inline uint64_t ROTL64(uint64_t x, int b) {
    return (x << b) | (x >> (64 - b));
}

static inline void SIPROUND(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3) {
    v0 += v1; v2 += v3; v1 = ROTL64(v1, 13); v3 = ROTL64(v3, 16);
    v1 ^= v0; v3 ^= v2; v0 = ROTL64(v0, 32);
    v2 += v1; v0 += v3; v1 = ROTL64(v1, 17); v3 = ROTL64(v3, 21);
    v1 ^= v2; v3 ^= v0; v2 = ROTL64(v2, 32);
}

static inline uint64_t siphash_core(const SipHashKey& key, uint64_t m, int c_rounds, int d_rounds) {
    uint64_t v0 = 0x736f6d6570736575ULL ^ key.k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ key.k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ key.k0;
    uint64_t v3 = 0x7465646279746573ULL ^ key.k1;

    // Process 8-byte message word m
    v3 ^= m;
    for (int i = 0; i < c_rounds; ++i) SIPROUND(v0, v1, v2, v3);
    v0 ^= m;

    // Final block with message length (8 bytes)
    const uint64_t b = (uint64_t)8 << 56; // no residual bytes
    v3 ^= b;
    for (int i = 0; i < c_rounds; ++i) SIPROUND(v0, v1, v2, v3);
    v0 ^= b;

    // Finalization
    v2 ^= 0xff;
    for (int i = 0; i < d_rounds; ++i) SIPROUND(v0, v1, v2, v3);

    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t siphash12(const SipHashKey& key, uint64_t nonce) {
    return siphash_core(key, nonce, 1, 2);
}

uint64_t siphash24(const SipHashKey& key, uint64_t nonce) {
    return siphash_core(key, nonce, 2, 4);
}

void siphash12_batch(const SipHashKey& key, const uint64_t* nonces, uint64_t* out, size_t count) {
    for (size_t i = 0; i < count; ++i) out[i] = siphash12(key, nonces[i]);
}

void siphash24_batch(const SipHashKey& key, const uint64_t* nonces, uint64_t* out, size_t count) {
    for (size_t i = 0; i < count; ++i) out[i] = siphash24(key, nonces[i]);
}

} // namespace cuckoo_sip
