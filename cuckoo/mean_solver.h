#ifndef CUCKOO_SIP_MEAN_SOLVER_H
#define CUCKOO_SIP_MEAN_SOLVER_H

#include <cstdint>
#include <vector>
#include <string>

#include "graph.h"

namespace cuckoo_sip {

struct MeanResult {
    bool success = false;
    std::vector<uint64_t> solution_edges;
    size_t rounds_run = 0;
    size_t alive_edges = 0;
    std::string note;
};

// Open-memory bucketed solver: trims by buckets on low bits of endpoints to accelerate degree counting.
class MeanSolver {
public:
    MeanSolver(const Params& params, uint32_t threads, uint32_t bucket_bits = 12);

    // Perform alternating side-based bucketed trimming for up to max_rounds, then attempt k-cycle recovery.
    MeanResult solve(uint32_t max_rounds = 8, uint32_t cycle_length = 42);

private:
    const Params p_;
    const uint32_t threads_;
    const uint32_t bucket_bits_;

    // Bitset helpers (like lean, but local to mean solver)
    static inline size_t words_for_bits(uint64_t nbits) { return static_cast<size_t>((nbits + 63ULL) / 64ULL); }
    static inline bool bit_get(const std::vector<uint64_t>& v, uint64_t idx) { return (v[idx >> 6] >> (idx & 63ULL)) & 1ULL; }
    static inline void bit_set(std::vector<uint64_t>& v, uint64_t idx) { v[idx >> 6] |= (1ULL << (idx & 63ULL)); }

    // Initialize all edges as alive (N bits set)
    void init_edge_alive(std::vector<uint64_t>& edge_alive) const;

    // One trimming pass on a single side using bucketed degree counting; returns kept edge count.
    uint64_t trim_side_bucketed(const std::vector<uint64_t>& edge_alive, std::vector<uint64_t>& new_edge_alive, int side) const;

    // Attempt cycle recovery for a target cycle length k using DSU+BFS on the forest of remaining edges.
    bool recover_cycle_k(const std::vector<uint64_t>& edge_alive, uint32_t k, std::vector<uint64_t>& solution) const;
};

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_MEAN_SOLVER_H
