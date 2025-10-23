#include "verify/verify.h"
#include <unordered_map>
#include <unordered_set>

namespace verify42 {

using namespace cuckoo;

bool verify_cycle_42(const Params& params, const std::vector<edge_t>& edges, std::string* err) {
  if (edges.size() != 42) {
    if (err) *err = "expected 42 edges, got " + std::to_string(edges.size());
    return false;
  }
  std::unordered_set<edge_t> uniq;
  for (auto e : edges) {
    if (!uniq.insert(e).second) {
      if (err) *err = "duplicate edge index in solution";
      return false;
    }
  }

  std::vector<node_t> us(42), vs(42);
  for (size_t i = 0; i < 42; ++i) {
    us[i] = endpoint(params, edges[i], 0);
    vs[i] = endpoint(params, edges[i], 1);
  }

  std::unordered_map<node_t, std::vector<int>> mapU;
  std::unordered_map<node_t, std::vector<int>> mapV;
  mapU.reserve(64); mapV.reserve(64);

  for (int i = 0; i < 42; ++i) {
    mapU[us[i]].push_back(i);
    mapV[vs[i]].push_back(i);
  }

  for (auto& kv : mapU) {
    if (kv.second.size() != 2) {
      if (err) *err = "side-0 node degree != 2";
      return false;
    }
  }
  for (auto& kv : mapV) {
    if (kv.second.size() != 2) {
      if (err) *err = "side-1 node degree != 2";
      return false;
    }
  }

  int start = 0;
  int curEdge = start;
  int curSide = 0;
  auto curNode = us[start];

  for (int step = 0; step < 42; ++step) {
    const auto& vec = (curSide == 0) ? mapU[curNode] : mapV[curNode];
    if (vec.size() != 2) {
      if (err) *err = "bad degree encountered in traversal";
      return false;
    }
    int nextEdge = (vec[0] == curEdge) ? vec[1] : vec[0];
    if (step == 41) {
      if (nextEdge != start) {
        if (err) *err = "did not return to start after 42 steps";
        return false;
      }
      break;
    }
    curEdge = nextEdge;
    curSide = 1 - curSide;
    curNode = (curSide == 0) ? us[curEdge] : vs[curEdge];
  }

  return true;
}

} // namespace verify42
