#ifndef SIPHASH12_H
#define SIPHASH12_H

#include <cstdint>
#include <cstddef>

namespace cuckoo_sip {

struct SipHashKey {
    uint64_t k0;
    uint64_t k1;
};

enum class SipHashVariant {
    SipHash12,
    SipHash24
};

// Scalar PRF on 64-bit nonce
uint64_t siphash12(const SipHashKey& key, uint64_t nonce);
uint64_t siphash24(const SipHashKey& key, uint64_t nonce);

// Batched PRF on an array of nonces
void siphash12_batch(const SipHashKey& key, const uint64_t* nonces, uint64_t* out, size_t count);
void siphash24_batch(const SipHashKey& key, const uint64_t* nonces, uint64_t* out, size_t count);

// Convenience: dispatch by variant
inline uint64_t siphash_dispatch(SipHashVariant v, const SipHashKey& key, uint64_t nonce) {
    return (v == SipHashVariant::SipHash12) ? siphash12(key, nonce) : siphash24(key, nonce);
}

} // namespace cuckoo_sip

#endif // SIPHASH12_H
