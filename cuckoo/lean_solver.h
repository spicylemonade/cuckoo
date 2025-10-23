#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "cuckoo/graph.h"

namespace cuckoo {

struct LeanSolverConfig {
  uint32_t trim_rounds = 32; // number of leaf-trimming rounds
  double memcap_bytes_per_edge = 1.0; // enforcement cap
};

struct Solution {
  bool found = false;
  std::vector<edge_t> edges; // if found, exactly 42 edge indices
};

class LeanSolver {
public:
  explicit LeanSolver(const LeanSolverConfig& cfg = {}) : cfg_(cfg) {}

  // Theoretical persistent memory usage per edge (bytes/edge) for trimming + recovery
  // edge_alive: N bits => N/8 bytes
  // node counters (per side): seen_once + seen_twice => 2*N bits per side => 4*N bits total => N/2 bytes
  // Total = N/8 + N/2 = 5N/8 bytes = 0.625 bytes/edge
  double theoretical_bytes_per_edge(uint32_t /*edge_bits*/) const { return 5.0 / 8.0; }

  bool solve(const Params& params, Solution& out_solution, std::string* log_msg = nullptr) const;

private:
  LeanSolverConfig cfg_;

  static inline bool get_bit(const std::vector<uint64_t>& bits, uint64_t idx) {
    return (bits[idx >> 6] >> (idx & 63)) & 1ULL;
  }
  static inline void set_bit(std::vector<uint64_t>& bits, uint64_t idx) {
    bits[idx >> 6] |= (1ULL << (idx & 63));
  }
  static inline void clear_bit(std::vector<uint64_t>& bits, uint64_t idx) {
    bits[idx >> 6] &= ~(1ULL << (idx & 63));
  }

  bool trim_rounds(const Params& params,
                   std::vector<uint64_t>& edge_alive,
                   std::vector<uint64_t>& once0,
                   std::vector<uint64_t>& twice0,
                   std::vector<uint64_t>& once1,
                   std::vector<uint64_t>& twice1) const;

  bool find_cycle_42_streaming(const Params& params,
                               const std::vector<uint64_t>& edge_alive,
                               std::vector<edge_t>& cycle_out) const;

  bool find_other_edge_for_node(const Params& params,
                                const std::vector<uint64_t>& edge_alive,
                                node_t node,
                                int side,
                                edge_t exclude_edge,
                                edge_t& next_edge) const;
};

} // namespace cuckoo
