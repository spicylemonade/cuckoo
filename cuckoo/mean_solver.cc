#include "cuckoo/mean_solver.h"
#include <chrono>

namespace cuckoo_sip {

MeanSolveResult MeanSolver::solve_42_cycle() {
  MeanSolveResult r;
  auto t0 = std::chrono::steady_clock::now();

  LeanSolver lean(G_, /*memcap_bpe*/ 8.0, /*trim_rounds*/ 12);
  auto res = lean.solve_42_cycle();

  r.found = res.found;
  r.cycle_edges = std::move(res.cycle_edges);
  auto t1 = std::chrono::steady_clock::now();
  r.seconds = std::chrono::duration<double>(t1 - t0).count();
  r.note = "mean solver placeholder (reuses lean impl)";
  return r;
}

} // namespace cuckoo_sip
