#pragma once
#include <cstdint>
#include <vector>
#include <utility>
#include <optional>
#include <string>
#include <unordered_map>
#include "sip12.h"

struct SolverConfig {
    uint32_t n;                 // N = 2^n edges
    uint32_t trim_rounds;       // number of trimming rounds
    uint32_t threads;           // threads (future use)
    sip12::Key128 key;          // siphash key
};

struct Proof {
    bool found;
    std::vector<uint64_t> edges; // 42 ascending edge indices when found
};

class CuckooSolver {
public:
    explicit CuckooSolver(const SolverConfig &cfg);

    // Run trimming, build reduced graph, and search for a 42-cycle
    Proof find_cycle42();

    // Deterministic endpoint mapping for edge index i and side (0 or 1)
    inline uint64_t endpoint(uint64_t side, uint64_t i) const {
        uint64_t h = sip12::siphash12_u64(side, i, cfg_.key);
        return h & node_mask_; // nodes_per_side == 2^n
    }

    // Verify a proof list of 42 edge indices
    bool verify_cycle42(const std::vector<uint64_t> &edges) const;

private:
    SolverConfig cfg_;
    uint64_t N_;                 // number of edges
    uint64_t nodes_per_side_;
    uint64_t node_mask_;

    // Edge liveness bitmap (1 bit per edge)
    std::vector<uint64_t> edge_alive_;

    inline bool edge_alive(uint64_t i) const {
        return (edge_alive_[i >> 6] >> (i & 63)) & 1ULL;
    }
    inline void set_edge_dead(uint64_t i) {
        edge_alive_[i >> 6] &= ~(1ULL << (i & 63));
    }
    inline void set_edge_alive(uint64_t i) {
        edge_alive_[i >> 6] |= (1ULL << (i & 63));
    }

    // Trimming
    void initialize_liveness();
    void trim_round_side(uint32_t side);

    // After trimming, build adjacency list for surviving edges
    void build_reduced_graph(std::vector<std::vector<std::pair<uint64_t,uint64_t>>> &adj) const;

    // DFS to find cycles; returns first 42-cycle
    std::optional<std::vector<uint64_t>> dfs_find_cycle42(const std::vector<std::vector<std::pair<uint64_t,uint64_t>>> &adj) const;
};
