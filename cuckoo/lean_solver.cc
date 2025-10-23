#include "cuckoo/lean_solver.h"
#include <chrono>
#include <algorithm>

namespace cuckoo_sip {

static inline uint64_t next_pow2(uint64_t x) {
  if (x <= 1) return 1;
  --x;
  x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16; x |= x >> 32;
  return x + 1;
}

LeanSolver::LeanSolver(const Graph& g, double memcap_bytes_per_edge, uint32_t trim_rounds)
  : G_(g), N_(g.N()), edge_bits_(g.edge_bits()), nodes_per_side_(1ULL << g.edge_bits()),
    memcap_bpe_(memcap_bytes_per_edge), trim_rounds_(trim_rounds) {
  edge_alive_.reset_size(N_);
  edge_alive_.set_all();

  deg1_side0_.reset_size(nodes_per_side_);
  deg2_side0_.reset_size(nodes_per_side_);
  deg1_side1_.reset_size(nodes_per_side_);
  deg2_side1_.reset_size(nodes_per_side_);
}

double LeanSolver::theoretical_mem_bytes_per_edge(uint64_t N, uint64_t nodes_per_side) {
  double edges_alive_bytes = (double)((N + 7) / 8);
  double deg_bitmaps_bits = (double)(nodes_per_side * 4ULL); // 2 bitmaps/side * 2 sides
  double deg_bytes = deg_bitmaps_bits / 8.0;
  double total_bytes = edges_alive_bytes + deg_bytes;
  return total_bytes / (double)N;
}

void LeanSolver::compute_degrees() {
  deg1_side0_.clear_all();
  deg2_side0_.clear_all();
  deg1_side1_.clear_all();
  deg2_side1_.clear_all();

  for (uint64_t i = 0; i < N_; ++i) {
    if (!edge_alive_.test(i)) continue;
    auto e = G_.endpoints(i);
    if (!deg1_side0_.test(e.u)) { deg1_side0_.set(e.u); }
    else if (!deg2_side0_.test(e.u)) { deg2_side0_.set(e.u); }

    if (!deg1_side1_.test(e.v)) { deg1_side1_.set(e.v); }
    else if (!deg2_side1_.test(e.v)) { deg2_side1_.set(e.v); }
  }
}

uint64_t LeanSolver::trim_leaves_once() {
  uint64_t removed = 0;
  for (uint64_t i = 0; i < N_; ++i) {
    if (!edge_alive_.test(i)) continue;
    auto e = G_.endpoints(i);
    const bool u_is_leaf = deg1_side0_.test(e.u) && !deg2_side0_.test(e.u);
    const bool v_is_leaf = deg1_side1_.test(e.v) && !deg2_side1_.test(e.v);
    if (u_is_leaf || v_is_leaf) { edge_alive_.reset(i); ++removed; }
  }
  return removed;
}

void LeanSolver::ParentMap::init_with_capacity(uint64_t capacity_pow2) {
  keys_.assign(capacity_pow2, 0U);
  vals_.assign(capacity_pow2, ParentEntry{0U, 0U});
  used_.assign(capacity_pow2, 0U);
  mask_ = capacity_pow2 - 1;
  size_ = 0;
}

bool LeanSolver::ParentMap::try_emplace(uint32_t node, uint32_t parent_node, uint32_t parent_edge) {
  uint64_t idx = (uint64_t)node * 11400714819323198485ull;
  idx &= mask_;
  for (uint64_t step = 0; step <= mask_; ++step) {
    uint64_t pos = (idx + step) & mask_;
    if (!used_[pos]) {
      used_[pos] = 1;
      keys_[pos] = node;
      vals_[pos] = ParentEntry{parent_node, parent_edge};
      ++size_;
      return true;
    }
    if (keys_[pos] == node) return false;
  }
  return false;
}

bool LeanSolver::ParentMap::get(uint32_t node, ParentEntry& out) const {
  uint64_t idx = (uint64_t)node * 11400714819323198485ull;
  idx &= mask_;
  for (uint64_t step = 0; step <= mask_; ++step) {
    uint64_t pos = (idx + step) & mask_;
    if (!used_[pos]) return false;
    if (keys_[pos] == node) { out = vals_[pos]; return true; }
  }
  return false;
}

bool LeanSolver::try_recover_cycle_42(std::vector<uint32_t>& out_edges) {
  const double base_bpe = theoretical_mem_bytes_per_edge(N_, nodes_per_side_);
  const double allowed_bytes_total = memcap_bpe_ * (double)N_;
  const double base_bytes_total = base_bpe * (double)N_;
  double budget_bytes = allowed_bytes_total - base_bytes_total;
  if (budget_bytes < 0) budget_bytes = 0;

  double bytes_per_entry = 26.0;
  uint64_t max_entries = (budget_bytes > 0) ? (uint64_t)(budget_bytes / bytes_per_entry) : 0ULL;
  if (max_entries < 1024) max_entries = 1024;

  auto cap_pow2 = next_pow2(max_entries * 2ULL);
  ParentMap p0, p1; p0.init_with_capacity(cap_pow2); p1.init_with_capacity(cap_pow2);

  auto build_path = [&](int side, uint32_t node, std::vector<uint32_t>& nodes, std::vector<uint32_t>& edges) {
    nodes.clear(); edges.clear();
    nodes.push_back(node);
    ParentEntry ent;
    while (true) {
      if (side == 0) {
        if (!p0.get(node, ent)) break;
      } else {
        if (!p1.get(node, ent)) break;
      }
      edges.push_back(ent.parent_edge);
      node = ent.parent_node;
      side ^= 1;
      nodes.push_back(node);
      if (nodes.size() > 2000000) break;
    }
  };

  std::vector<uint32_t> nodesU, edgesU, nodesV, edgesV;

  for (uint64_t i = 0; i < N_; ++i) {
    if (!edge_alive_.test(i)) continue;
    auto e = G_.endpoints(i);

    build_path(0, e.u, nodesU, edgesU);
    build_path(1, e.v, nodesV, edgesV);

    while (!nodesU.empty() && !nodesV.empty() && nodesU.back() == nodesV.back()) {
      if (!edgesU.empty()) edgesU.pop_back();
      if (!edgesV.empty()) edgesV.pop_back();
      nodesU.pop_back();
      nodesV.pop_back();
    }

    uint32_t lenU = (uint32_t)edgesU.size();
    uint32_t lenV = (uint32_t)edgesV.size();
    uint32_t cycle_len = lenU + lenV + 1;

    if (cycle_len == 42) {
      out_edges.clear();
      for (uint32_t eidx : edgesU) out_edges.push_back(eidx);
      out_edges.push_back((uint32_t)i);
      for (int k = (int)edgesV.size() - 1; k >= 0; --k) out_edges.push_back(edgesV[(size_t)k]);
      if (out_edges.size() == 42) return true;
      out_edges.clear();
    }

    uint32_t rootU = nodesU.empty() ? e.u : nodesU.back();
    uint32_t rootV = nodesV.empty() ? e.v : nodesV.back();

    (void)p0.try_emplace(rootU, rootV, (uint32_t)i);
    (void)p1.try_emplace(rootV, rootU, (uint32_t)i);
  }

  return false;
}

SolveResult LeanSolver::solve_42_cycle() {
  SolveResult res;
  auto t0 = std::chrono::steady_clock::now();
  res.stats.edges_initial = N_;

  for (uint32_t r = 0; r < trim_rounds_; ++r) {
    compute_degrees();
    uint64_t removed = trim_leaves_once();
    res.stats.rounds_performed++;
    if (removed == 0) break;
  }
  res.stats.edges_remaining = edge_alive_.count();

  std::vector<uint32_t> cycle;
  if (try_recover_cycle_42(cycle)) {
    res.found = true;
    res.cycle_edges = std::move(cycle);
  } else {
    res.found = false;
  }

  auto t1 = std::chrono::steady_clock::now();
  res.stats.seconds = std::chrono::duration<double>(t1 - t0).count();
  res.stats.mem_bytes_per_edge = theoretical_mem_bytes_per_edge(N_, nodes_per_side_);
  res.stats.note = "lean trimming + bounded forest recovery";
  return res;
}

} // namespace cuckoo_sip
