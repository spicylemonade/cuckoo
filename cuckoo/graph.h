#pragma once
#include <cstdint>
#include <utility>
#include "src/siphash.h"

namespace cuckoo_sip {

using node_t = uint32_t; // up to edge_bits <= 32

struct Params {
  uint32_t edge_bits{29};
  uint64_t N{1ULL << 29};
  Key128 key{0,0};
  SipVariant hash_variant{SipVariant::SIPHASH_12};
};

struct EdgeEndpoints { node_t u; node_t v; };

class Graph {
 public:
  explicit Graph(const Params& p) : params_(p), node_mask_((1ULL << p.edge_bits) - 1ULL) {}

  inline node_t endpoint_u(uint64_t i) const {
    return static_cast<node_t>(hash_nonce((i << 1) | 0ULL) & node_mask_);
  }
  inline node_t endpoint_v(uint64_t i) const {
    return static_cast<node_t>(hash_nonce((i << 1) | 1ULL) & node_mask_);
  }
  inline EdgeEndpoints endpoints(uint64_t i) const { return EdgeEndpoints{endpoint_u(i), endpoint_v(i)}; }

  inline uint64_t N() const { return params_.N; }
  inline uint32_t edge_bits() const { return params_.edge_bits; }
  inline const Params& params() const { return params_; }

 private:
  uint64_t hash_nonce(uint64_t nonce) const {
    if (params_.hash_variant == SipVariant::SIPHASH_12) return siphash12_u64(params_.key, nonce);
    return siphash24_u64(params_.key, nonce);
  }

  Params params_;
  uint64_t node_mask_;
};

} // namespace cuckoo_sip
