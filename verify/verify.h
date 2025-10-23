#ifndef CUCKOO_SIP_VERIFY_H
#define CUCKOO_SIP_VERIFY_H

#include <vector>
#include <string>

#include "cuckoo/graph.h"

namespace cuckoo_sip {

// Verifies that the provided edge indices form a single cycle of length k on the graph defined by Params.
// Returns true on success; err contains a message on failure.
bool verify_cycle_k(const Params& p, const std::vector<uint64_t>& edges, size_t k, std::string* err);

// Wrapper for exactly 42 edges (for bounty conformance)
inline bool verify_cycle_42(const Params& p, const std::vector<uint64_t>& edges, std::string* err) {
    return verify_cycle_k(p, edges, 42, err);
}

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_VERIFY_H
