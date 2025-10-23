#ifndef CUCKOO_SIP_GRAPH_H
#define CUCKOO_SIP_GRAPH_H

#include <cstdint>
#include <cstddef>

#include "siphash12.h"

namespace cuckoo_sip {

using node_t = uint32_t; // supports up to 2^32 nodes per side

struct Params {
    uint32_t edge_bits = 29;           // n: N = 2^n edges
    uint64_t N = 1ULL << 29;           // edges
    uint64_t node_mask = (1ULL << 29) - 1ULL; // mask for nodes
    SipHashKey key{};                  // SipHash key
    SipHashVariant variant = SipHashVariant::SipHash12; // hash variant
};

inline void set_edge_bits(Params& p, uint32_t edge_bits) {
    p.edge_bits = edge_bits;
    p.N = (edge_bits >= 63) ? 0 : (1ULL << edge_bits);
    p.node_mask = (edge_bits >= 63) ? ~0ULL : ((1ULL << edge_bits) - 1ULL);
}

inline uint64_t hash_nonce(const Params& p, uint64_t nonce) {
    if (p.variant == SipHashVariant::SipHash12) return siphash12(p.key, nonce);
    else return siphash24(p.key, nonce);
}

inline node_t endpoint(const Params& p, uint64_t i, int side) {
    // side = 0 => u, side = 1 => v
    const uint64_t x = (i << 1) | (static_cast<uint64_t>(side) & 1ULL);
    const uint64_t h = hash_nonce(p, x);
    return static_cast<node_t>(h & p.node_mask);
}

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_GRAPH_H
