#pragma once
#include <cstdint>
#include "src/siphash.h"

namespace cuckoo {

using node_t = uint64_t;
using edge_t = uint64_t;

enum class HashVariant {
  Sip12,
  Sip24
};

struct Params {
  uint32_t edge_bits = 29; // default
  uint64_t N = 1ULL << 29;
  uint64_t node_mask = (1ULL << 29) - 1ULL;
  siphash::Key key{0,0};
  HashVariant variant = HashVariant::Sip12;
};

inline Params make_params(uint32_t edge_bits, const siphash::Key& key, HashVariant variant) {
  Params p;
  p.edge_bits = edge_bits;
  p.N = (edge_bits >= 64 ? 0ULL : (1ULL << edge_bits));
  p.node_mask = (edge_bits >= 64 ? ~0ULL : ((1ULL << edge_bits) - 1ULL));
  p.key = key;
  p.variant = variant;
  return p;
}

inline node_t endpoint(const Params& params, edge_t i, int side) {
  const uint64_t nonce = (i << 1) | (uint64_t)(side & 1);
  if (params.variant == HashVariant::Sip12) {
    return siphash::siphash12_u64(params.key, nonce) & params.node_mask;
  } else {
    return siphash::siphash24_u64(params.key, nonce) & params.node_mask;
  }
}

} // namespace cuckoo
