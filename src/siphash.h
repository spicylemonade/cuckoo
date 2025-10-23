#pragma once
#include <cstdint>
#include <cstddef>

namespace cuckoo_sip {

struct Key128 { uint64_t k0; uint64_t k1; };

enum class SipVariant { SIPHASH_12 = 0, SIPHASH_24 = 1 };

uint64_t siphash_u64_rounds(const Key128& key, uint64_t nonce, int c, int d);

inline uint64_t siphash12_u64(const Key128& key, uint64_t nonce) {
  return siphash_u64_rounds(key, nonce, 1, 2);
}
inline uint64_t siphash24_u64(const Key128& key, uint64_t nonce) {
  return siphash_u64_rounds(key, nonce, 2, 4);
}

void siphash12_batch(const Key128& key, const uint64_t* nonces, uint64_t* out, size_t count);
void siphash24_batch(const Key128& key, const uint64_t* nonces, uint64_t* out, size_t count);

} // namespace cuckoo_sip
