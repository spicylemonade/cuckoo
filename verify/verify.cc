#include "verify/verify.h"
#include <unordered_set>

namespace cuckoo_sip {

VerifyResult verify_42_cycle(const Graph& G, const std::vector<uint32_t>& edges) {
  VerifyResult vr;
  if (edges.size() != 42) {
    vr.ok = false;
    vr.error = "expected 42 edge indices";
    return vr;
  }
  std::unordered_set<uint32_t> uniq;
  for (uint32_t e : edges) uniq.insert(e);
  if (uniq.size() != edges.size()) {
    vr.ok = false;
    vr.error = "duplicate edge indices in cycle";
    return vr;
  }

  std::vector<std::pair<node_t,node_t>> ev(42);
  for (size_t i = 0; i < 42; ++i) {
    auto ep = G.endpoints(edges[i]);
    ev[i] = {ep.u, ep.v};
  }

  std::vector<int> used(42, 0);
  std::vector<std::pair<node_t,node_t>> cyc;
  cyc.reserve(42);

  cyc.push_back(ev[0]);
  used[0] = 1;
  node_t cur = ev[0].v;
  bool next_is_u = true; // match next edge by v==cur, then advance to its u

  for (int step = 1; step < 42; ++step) {
    bool advanced = false;
    for (int j = 1; j < 42; ++j) {
      if (used[j]) continue;
      if (next_is_u) {
        if (ev[j].v == cur) { cur = ev[j].u; used[j] = 1; cyc.push_back(ev[j]); next_is_u = false; advanced = true; break; }
      } else {
        if (ev[j].u == cur) { cur = ev[j].v; used[j] = 1; cyc.push_back(ev[j]); next_is_u = true; advanced = true; break; }
      }
    }
    if (!advanced) {
      vr.ok = false;
      vr.error = "edges do not chain into a single alternating cycle";
      return vr;
    }
  }

  // After 41 steps, cur should be a U-side node and should equal the starting u to close the cycle
  if (cur != ev[0].u) {
    vr.ok = false;
    vr.error = "cycle does not close";
    return vr;
  }

  vr.ok = true;
  vr.error.clear();
  return vr;
}

} // namespace cuckoo_sip
