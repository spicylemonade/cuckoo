#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <utility>
#include "cuckoo/graph.h"
#include "src/bitset.h"

namespace cuckoo_sip {

struct SolveStats {
  uint64_t rounds_performed{0};
  uint64_t edges_initial{0};
  uint64_t edges_remaining{0};
  double seconds{0.0};
  double mem_bytes_per_edge{0.0};
  std::string note;
};

struct SolveResult {
  bool found{false};
  std::vector<uint32_t> cycle_edges; // 42 indices
  SolveStats stats;
};

class LeanSolver {
 public:
  LeanSolver(const Graph& g, double memcap_bytes_per_edge = 1.0, uint32_t trim_rounds = 8);

  SolveResult solve_42_cycle();

  static double theoretical_mem_bytes_per_edge(uint64_t N, uint64_t nodes_per_side);

 private:
  const Graph& G_;
  const uint64_t N_;
  const uint32_t edge_bits_;
  const uint64_t nodes_per_side_;
  const double memcap_bpe_;
  const uint32_t trim_rounds_;

  DynamicBitset edge_alive_;
  DynamicBitset deg1_side0_;
  DynamicBitset deg2_side0_;
  DynamicBitset deg1_side1_;
  DynamicBitset deg2_side1_;

  void compute_degrees();
  uint64_t trim_leaves_once();

  struct ParentEntry { uint32_t parent_node; uint32_t parent_edge; };
  class ParentMap {
   public:
    ParentMap() : mask_(0), size_(0) {}
    void init_with_capacity(uint64_t capacity_pow2);
    bool try_emplace(uint32_t node, uint32_t parent_node, uint32_t parent_edge);
    bool get(uint32_t node, ParentEntry& out) const;
    uint64_t capacity() const { return keys_.size(); }
    uint64_t size() const { return size_; }
   private:
    std::vector<uint32_t> keys_;
    std::vector<ParentEntry> vals_;
    std::vector<uint8_t> used_;
    uint64_t mask_;
    uint64_t size_;
  };

  bool try_recover_cycle_42(std::vector<uint32_t>& out_edges);
};

} // namespace cuckoo_sip
