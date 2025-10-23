#ifndef CUCKOO_SIP_LEAN_SOLVER_H
#define CUCKOO_SIP_LEAN_SOLVER_H

#include <cstdint>
#include <vector>
#include <string>

#include "graph.h"

namespace cuckoo_sip {

struct LeanResult {
    bool success = false;
    std::vector<uint64_t> solution_edges; // k edge indices if success
    size_t rounds_run = 0;
    size_t alive_edges = 0;
    double mem_bytes_per_edge = 0.0; // computed persistent memory usage
    std::string note;
};

class LeanSolver {
public:
    LeanSolver(const Params& params, uint32_t threads, double memcap_bytes_per_edge = 1.0);

    // Attempts to find a cycle of given length. Returns LeanResult with status and info.
    LeanResult solve(uint32_t max_rounds = 256, uint32_t cycle_length = 42);

    // Theoretical persistent memory usage (bitsets)
    size_t memory_usage_bytes() const;
    double mem_bytes_per_edge() const;

private:
    const Params p_;
    const uint32_t threads_;
    const double memcap_bpe_;

    // Bitset helpers
    static inline size_t words_for_bits(uint64_t nbits) { return static_cast<size_t>((nbits + 63ULL) / 64ULL); }
    static inline bool bit_get(const std::vector<uint64_t>& v, uint64_t idx) {
        return (v[idx >> 6] >> (idx & 63ULL)) & 1ULL;
    }
    static inline void bit_set(std::vector<uint64_t>& v, uint64_t idx) {
        v[idx >> 6] |= (1ULL << (idx & 63ULL));
    }
    static inline void bit_clear(std::vector<uint64_t>& v, uint64_t idx) {
        v[idx >> 6] &= ~(1ULL << (idx & 63ULL));
    }

    // Allocate and initialize masks
    void init_edge_alive(std::vector<uint64_t>& edge_alive) const;

    // One trimming iteration computing nonleaf sets for both sides and filtering edges with both endpoints nonleaf.
    uint64_t trim_round_both(const std::vector<uint64_t>& edge_alive,
                             std::vector<uint64_t>& new_edge_alive,
                             std::vector<uint64_t>& seen0,
                             std::vector<uint64_t>& nonleaf0,
                             std::vector<uint64_t>& seen1,
                             std::vector<uint64_t>& nonleaf1) const;

    // Alternate-side leaf trimming: trim on a single side (0 or 1), keeping edges whose endpoint on that side is nonleaf.
    uint64_t trim_round_side(const std::vector<uint64_t>& edge_alive,
                             std::vector<uint64_t>& new_edge_alive,
                             std::vector<uint64_t>& seen_side,
                             std::vector<uint64_t>& nonleaf_side,
                             int side) const;

    // Attempt cycle recovery for a target cycle length k using DSU+BFS on the forest of remaining edges.
    bool recover_cycle_k(const std::vector<uint64_t>& edge_alive, uint32_t k, std::vector<uint64_t>& solution) const;
};

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_LEAN_SOLVER_H
