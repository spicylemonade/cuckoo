#pragma once
#include <cstdint>
#include <cstddef>

namespace siphash {

struct Key {
  uint64_t k0;
  uint64_t k1;
};

enum class Variant {
  Sip12,
  Sip24
};

uint64_t siphash_c_d_u64(const Key& key, uint64_t m, int c, int d);

inline uint64_t siphash12_u64(const Key& key, uint64_t m) {
  return siphash_c_d_u64(key, m, 1, 2);
}

inline uint64_t siphash24_u64(const Key& key, uint64_t m) {
  return siphash_c_d_u64(key, m, 2, 4);
}

inline uint64_t siphash_u64(const Key& key, uint64_t m, Variant v) {
  return (v == Variant::Sip12) ? siphash12_u64(key, m) : siphash24_u64(key, m);
}

void siphash12_batch_u64(const Key& key, const uint64_t* in, uint64_t* out, size_t count);
void siphash24_batch_u64(const Key& key, const uint64_t* in, uint64_t* out, size_t count);

} // namespace siphash
