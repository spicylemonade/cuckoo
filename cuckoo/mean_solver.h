#pragma once
#include <vector>
#include <string>
#include "cuckoo/graph.h"

namespace cuckoo {

struct MeanSolverConfig {
};

struct MeanSolution {
  bool found = false;
  std::vector<edge_t> edges;
  std::string note;
};

class MeanSolver {
public:
  explicit MeanSolver(const MeanSolverConfig& /*cfg*/ = {}) {}
  bool solve(const Params& /*params*/, MeanSolution& out_solution, std::string* log_msg = nullptr) const {
    out_solution.found = false;
    out_solution.note = "mean solver not implemented";
    if (log_msg) *log_msg = out_solution.note;
    return false;
  }
};

} // namespace cuckoo
