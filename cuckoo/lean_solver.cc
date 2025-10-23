#include "cuckoo/lean_solver.h"
#include <cstring>
#include <chrono>

namespace cuckoo {

bool LeanSolver::trim_rounds(const Params& params,
                             std::vector<uint64_t>& edge_alive,
                             std::vector<uint64_t>& once0,
                             std::vector<uint64_t>& twice0,
                             std::vector<uint64_t>& once1,
                             std::vector<uint64_t>& twice1) const {
  const uint64_t N = params.N;
  const size_t node_words = (N + 63) >> 6;
  (void)node_words; // unused var safeguard

  bool any_removed_total = false;

  for (uint32_t round = 0; round < cfg_.trim_rounds; ++round) {
    std::memset(once0.data(), 0, ((N + 63) >> 6) * sizeof(uint64_t));
    std::memset(twice0.data(), 0, ((N + 63) >> 6) * sizeof(uint64_t));
    std::memset(once1.data(), 0, ((N + 63) >> 6) * sizeof(uint64_t));
    std::memset(twice1.data(), 0, ((N + 63) >> 6) * sizeof(uint64_t));

    for (uint64_t i = 0; i < N; ++i) {
      if (!get_bit(edge_alive, i)) continue;
      node_t u = endpoint(params, i, 0);
      node_t v = endpoint(params, i, 1);

      size_t uidx = u >> 6; uint64_t umask = 1ULL << (u & 63);
      if (once0[uidx] & umask) {
        twice0[uidx] |= umask;
      } else {
        once0[uidx] |= umask;
      }

      size_t vidx = v >> 6; uint64_t vmask = 1ULL << (v & 63);
      if (once1[vidx] & vmask) {
        twice1[vidx] |= vmask;
      } else {
        once1[vidx] |= vmask;
      }
    }

    bool any_removed = false;
    for (uint64_t i = 0; i < N; ++i) {
      if (!get_bit(edge_alive, i)) continue;
      node_t u = endpoint(params, i, 0);
      node_t v = endpoint(params, i, 1);

      size_t uidx = u >> 6; uint64_t umask = 1ULL << (u & 63);
      bool leaf_u = (once0[uidx] & umask) && !(twice0[uidx] & umask);

      size_t vidx = v >> 6; uint64_t vmask = 1ULL << (v & 63);
      bool leaf_v = (once1[vidx] & vmask) && !(twice1[vidx] & vmask);

      if (leaf_u || leaf_v) {
        clear_bit(edge_alive, i);
        any_removed = true;
      }
    }

    any_removed_total = any_removed_total || any_removed;
    if (!any_removed) {
      break;
    }
  }

  return any_removed_total;
}

bool LeanSolver::find_other_edge_for_node(const Params& params,
                                          const std::vector<uint64_t>& edge_alive,
                                          node_t node,
                                          int side,
                                          edge_t exclude_edge,
                                          edge_t& next_edge) const {
  const uint64_t N = params.N;
  for (uint64_t j = 0; j < N; ++j) {
    if (j == exclude_edge) continue;
    if (!get_bit(edge_alive, j)) continue;
    if (side == 0) {
      node_t uj = endpoint(params, j, 0);
      if (uj == node) { next_edge = j; return true; }
    } else {
      node_t vj = endpoint(params, j, 1);
      if (vj == node) { next_edge = j; return true; }
    }
  }
  return false;
}

bool LeanSolver::find_cycle_42_streaming(const Params& params,
                                         const std::vector<uint64_t>& edge_alive,
                                         std::vector<edge_t>& cycle_out) const {
  const uint64_t N = params.N;
  for (uint64_t i = 0; i < N; ++i) {
    if (!get_bit(edge_alive, i)) continue;

    node_t start_u = endpoint(params, i, 0);
    edge_t current_edge = i;
    int current_side = 0;
    node_t current_node = start_u;

    std::vector<edge_t> path;
    path.reserve(42);
    path.push_back(i);

    bool failed = false;
    for (int step = 0; step < 42; ++step) {
      edge_t next_edge;
      if (!find_other_edge_for_node(params, edge_alive, current_node, current_side, current_edge, next_edge)) {
        failed = true;
        break;
      }
      if (step == 41) {
        if (next_edge != i) {
          failed = true;
        }
        break;
      }
      path.push_back(next_edge);
      if (current_side == 0) {
        current_node = endpoint(params, next_edge, 1);
        current_side = 1;
      } else {
        current_node = endpoint(params, next_edge, 0);
        current_side = 0;
      }
      current_edge = next_edge;
    }

    if (!failed && (int)path.size() == 42) {
      cycle_out = path;
      return true;
    }
  }
  return false;
}

bool LeanSolver::solve(const Params& params, Solution& out_solution, std::string* log_msg) const {
  double bpe = theoretical_bytes_per_edge(params.edge_bits);
  if (bpe > cfg_.memcap_bytes_per_edge) {
    if (log_msg) *log_msg = "LeanSolver memory cap violated: required " + std::to_string(bpe) + " B/edge > cap " + std::to_string(cfg_.memcap_bytes_per_edge);
    return false;
  }

  const uint64_t N = params.N;
  const size_t edge_words = (N + 63) >> 6;
  const size_t node_words = (N + 63) >> 6;
  (void)node_words;

  std::vector<uint64_t> edge_alive(edge_words, ~0ULL);
  if ((N & 63ULL) != 0ULL) {
    edge_alive[edge_words - 1] &= ((1ULL << (N & 63ULL)) - 1ULL);
  }

  std::vector<uint64_t> once0((N + 63) >> 6), twice0((N + 63) >> 6), once1((N + 63) >> 6), twice1((N + 63) >> 6);

  trim_rounds(params, edge_alive, once0, twice0, once1, twice1);

  std::vector<edge_t> cycle;
  bool found = find_cycle_42_streaming(params, edge_alive, cycle);
  out_solution.found = found;
  out_solution.edges = found ? cycle : std::vector<edge_t>{};

  if (!found) {
    if (log_msg) *log_msg = "No 42-cycle found";
  }

  return found;
}

} // namespace cuckoo
