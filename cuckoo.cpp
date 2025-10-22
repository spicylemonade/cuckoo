#include "cuckoo.h"
#include <algorithm>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cassert>
#include <chrono>
#include <iostream>

CuckooSolver::CuckooSolver(const SolverConfig &cfg) : cfg_(cfg) {
    N_ = 1ULL << cfg_.n;
    nodes_per_side_ = 1ULL << cfg_.n; // classic Cuckatoo uses 2^n nodes per side here
    node_mask_ = nodes_per_side_ - 1ULL;
    size_t words = (N_ + 63) >> 6;
    edge_alive_.assign(words, 0);
}

void CuckooSolver::initialize_liveness() {
    std::fill(edge_alive_.begin(), edge_alive_.end(), 0);
    for (uint64_t i = 0; i < N_; ++i) set_edge_alive(i);
}

void CuckooSolver::trim_round_side(uint32_t side) {
    // Count degrees per node on this side, considering only alive edges
    std::vector<uint16_t> deg(nodes_per_side_, 0);
    // Phase 1: count
    for (uint64_t i = 0; i < N_; ++i) {
        if (!edge_alive(i)) continue;
        uint64_t u = endpoint(side, i);
        if (deg[u] != 0xFFFF) deg[u]++;
    }
    // Phase 2: kill edges whose endpoint has degree == 1
    for (uint64_t i = 0; i < N_; ++i) {
        if (!edge_alive(i)) continue;
        uint64_t u = endpoint(side, i);
        if (deg[u] == 1) set_edge_dead(i);
    }
}

void CuckooSolver::build_reduced_graph(std::vector<std::vector<std::pair<uint64_t,uint64_t>>> &adj) const {
    // adjacency over left side nodes; pair is (neighbor node on right, edge index)
    adj.assign(nodes_per_side_, {});
    for (uint64_t i = 0; i < N_; ++i) {
        if (!edge_alive(i)) continue;
        uint64_t u = endpoint(0, i);
        uint64_t v = endpoint(1, i);
        adj[u].emplace_back(v, i);
    }
}

std::optional<std::vector<uint64_t>> CuckooSolver::dfs_find_cycle42(const std::vector<std::vector<std::pair<uint64_t,uint64_t>>> &adj) const {
    // Build full bipartite adjacency with both sides so DFS alternates sides.
    // We'll map right nodes to indices offset by nodes_per_side_ to keep visited arrays simple.
    uint64_t L = nodes_per_side_;
    uint64_t R = nodes_per_side_;
    uint64_t total_nodes = L + R;

    // Build adjacency lists over unified node space [0..L+R)
    std::vector<std::vector<std::pair<uint64_t,uint64_t>>> g(total_nodes);
    for (uint64_t u = 0; u < L; ++u) {
        for (auto &p : adj[u]) {
            uint64_t v = p.first;
            uint64_t e = p.second;
            uint64_t vidx = L + v;
            g[u].emplace_back(vidx, e);
            g[vidx].emplace_back(u, e);
        }
    }

    std::vector<int> parent_node(total_nodes, -1);
    std::vector<uint64_t> parent_edge(total_nodes, UINT64_C(0xffffffffffffffff));
    std::vector<char> seen(total_nodes, 0);

    // Depth-first search from each left node with degree>0
    std::vector<uint64_t> stack_nodes;
    stack_nodes.reserve(86);
    for (uint64_t s = 0; s < L; ++s) {
        if (g[s].empty()) continue;
        std::fill(seen.begin(), seen.end(), 0);
        std::fill(parent_node.begin(), parent_node.end(), -1);
        std::fill(parent_edge.begin(), parent_edge.end(), UINT64_C(0xffffffffffffffff));

        // iterative DFS with stack of (node, next neighbor index)
        std::vector<size_t> it_idx(total_nodes, 0);
        std::vector<uint64_t> order; order.reserve(86);
        std::vector<uint64_t> order_edges; order_edges.reserve(86);

        uint64_t curr = s;
        seen[curr] = 1;
        order.push_back(curr);
        while (true) {
            auto &nbrs = g[curr];
            bool advanced = false;
            while (it_idx[curr] < nbrs.size()) {
                auto [nxt, eidx] = nbrs[it_idx[curr]++];
                if (!seen[nxt]) {
                    parent_node[nxt] = (int)curr;
                    parent_edge[nxt] = eidx;
                    seen[nxt] = 1;
                    curr = nxt;
                    order.push_back(curr);
                    advanced = true;
                    break;
                } else {
                    // Found a back-edge: if nxt is on current path, we found a cycle
                    // Reconstruct cycle
                    int t = (int)curr;
                    std::unordered_set<uint64_t> path_nodes;
                    for (int x = t; x != -1; x = parent_node[x]) {
                        path_nodes.insert((uint64_t)x);
                        if (x == (int)nxt) break;
                    }
                    if (path_nodes.count(nxt)) {
                        // Extract edge list from nxt->curr via parents
                        std::vector<uint64_t> cyc_edges;
                        int x = (int)curr;
                        while (x != (int)nxt && x != -1) {
                            cyc_edges.push_back(parent_edge[x]);
                            x = parent_node[x];
                        }
                        cyc_edges.push_back(eidx); // edge closing the cycle
                        // The cycle length is number of edges
                        if (cyc_edges.size() == 42) {
                            std::sort(cyc_edges.begin(), cyc_edges.end());
                            return cyc_edges;
                        }
                    }
                }
            }
            if (advanced) continue;
            if (curr == s) break; // backtracked all the way
            // backtrack
            int p = parent_node[curr];
            curr = (uint64_t)p;
        }
    }
    return std::nullopt;
}

Proof CuckooSolver::find_cycle42() {
    initialize_liveness();
    for (uint32_t r = 0; r < cfg_.trim_rounds; ++r) {
        trim_round_side(0);
        trim_round_side(1);
    }
    std::vector<std::vector<std::pair<uint64_t,uint64_t>>> adj;
    build_reduced_graph(adj);
    auto cyc = dfs_find_cycle42(adj);
    if (!cyc) return Proof{false, {}};
    // ensure edges sorted ascending
    auto edges = *cyc;
    std::sort(edges.begin(), edges.end());
    return Proof{true, edges};
}

bool CuckooSolver::verify_cycle42(const std::vector<uint64_t> &edges) const {
    // Basic checks
    if (edges.size() != 42) return false;
    if (N_ == 0) return false;
    // Check sorted ascending and within range, and uniqueness
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i] >= N_) return false;
        if (i > 0 && edges[i] <= edges[i-1]) return false;
    }

    // Build the cycle walk by recomputing endpoints
    // We'll construct adjacency maps from left->(right,edge) and right->(left,edge)
    std::unordered_multimap<uint64_t, std::pair<uint64_t,uint64_t>> L2R;
    std::unordered_multimap<uint64_t, std::pair<uint64_t,uint64_t>> R2L;

    L2R.reserve(42);
    R2L.reserve(42);

    for (uint64_t e : edges) {
        uint64_t u = endpoint(0, e);
        uint64_t v = endpoint(1, e);
        L2R.emplace(u, std::make_pair(v, e));
        R2L.emplace(v, std::make_pair(u, e));
    }

    // In a simple 42-edge cycle, each participating node has degree exactly 2 in the composed graph,
    // but since we only include 42 edges, each left node and right node that participate should have degree 2 as well (counting within the proof).
    // We'll traverse the cycle: pick the smallest edge as a start and follow alternately L->R->L...
    uint64_t start_e = edges[0];
    uint64_t start_u = endpoint(0, start_e);
    uint64_t start_v = endpoint(1, start_e);

    // We'll try both directions starting from (u->v) and (v->u)
    auto follow = [&](uint64_t u0, uint64_t v0) -> bool {
        // Use sets of used edges to ensure we use each exactly once
        std::unordered_set<uint64_t> used;
        used.reserve(64);
        uint64_t u = u0;
        uint64_t v = v0;
        uint64_t last_e = UINT64_C(0xffffffffffffffff);
        // consume first edge (u,v)
        // Find e between u and v (there should be exactly one in our proof set)
        auto range = L2R.equal_range(u);
        uint64_t e_found = UINT64_C(0xffffffffffffffff);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.first == v) { e_found = it->second.second; break; }
        }
        if (e_found == UINT64_C(0xffffffffffffffff)) return false;
        used.insert(e_found);
        last_e = e_found;

        // Now walk the cycle using the other edge incident to each node
        for (int step = 1; step < 42; ++step) {
            // At right node v, choose the other edge not equal to last_e
            auto rrange = R2L.equal_range(v);
            uint64_t next_u = UINT64_C(0xffffffffffffffff);
            uint64_t e2 = UINT64_C(0xffffffffffffffff);
            int cnt = 0;
            for (auto it = rrange.first; it != rrange.second; ++it) {
                cnt++;
                if (it->second.second != last_e && used.count(it->second.second) == 0) {
                    next_u = it->second.first;
                    e2 = it->second.second;
                }
            }
            if (cnt < 1) return false; // no edge from this v in proof
            if (e2 == UINT64_C(0xffffffffffffffff)) return false; // couldn't find the other unused edge
            used.insert(e2);
            last_e = e2;
            u = next_u;

            // From left node u, choose the other edge not equal to last_e
            auto lrange = L2R.equal_range(u);
            uint64_t next_v = UINT64_C(0xffffffffffffffff);
            uint64_t e3 = UINT64_C(0xffffffffffffffff);
            cnt = 0;
            for (auto it = lrange.first; it != lrange.second; ++it) {
                cnt++;
                if (it->second.second != last_e && used.count(it->second.second) == 0) {
                    next_v = it->second.first;
                    e3 = it->second.second;
                }
            }
            if (cnt < 1) return false;
            if (e3 == UINT64_C(0xffffffffffffffff)) return false;
            used.insert(e3);
            last_e = e3;
            v = next_v;
        }
        // After 42 steps (used all edges), we should be back at the starting node pair
        return (u == u0) && (v == v0);
    };

    bool ok = follow(start_u, start_v);
    if (!ok) {
        // Try the other orientation
        ok = follow(start_u, start_v);
    }
    return ok;
}
