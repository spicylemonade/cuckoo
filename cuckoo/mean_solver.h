#pragma once
#include <vector>
#include <string>
#include "cuckoo/graph.h"
#include "cuckoo/lean_solver.h"

namespace cuckoo_sip {

struct MeanSolveResult {
  bool found{false};
  std::vector<uint32_t> cycle_edges;
  double seconds{0.0};
  std::string note;
};

class MeanSolver {
 public:
  explicit MeanSolver(const Graph& g) : G_(g) {}
  MeanSolveResult solve_42_cycle();
 private:
  const Graph& G_;
};

} // namespace cuckoo_sip
