#include "verify.h"

#include <unordered_set>

namespace cuckoo_sip {

bool verify_cycle_k(const Params& p, const std::vector<uint64_t>& edges, size_t k, std::string* err) {
    if (edges.size() != k) {
        if (err) *err = "Edge count does not match cycle length";
        return false;
    }
    // Check duplicates in edge indices
    {
        std::unordered_set<uint64_t> seen;
        seen.reserve(edges.size() * 2);
        for (auto e : edges) {
            if (e >= p.N) { if (err) *err = "Edge index out of range"; return false; }
            if (!seen.insert(e).second) {
                if (err) *err = "Duplicate edge index";
                return false;
            }
        }
    }

    // Compute endpoints
    std::vector<node_t> us(k), vs(k);
    for (size_t i = 0; i < k; ++i) {
        const uint64_t ei = edges[i];
        us[i] = endpoint(p, ei, 0);
        vs[i] = endpoint(p, ei, 1);
    }

    // Walk alternating sides greedily, ensuring all edges are used exactly once
    std::vector<bool> used(k, false);
    size_t used_cnt = 0;
    node_t cur_u = us[0];
    node_t cur_v = vs[0];
    used[0] = true; used_cnt = 1;

    for (size_t step = 1; step < k; ++step) {
        bool found = false;
        if (step & 1) {
            // Match on v (stay on side-1 node)
            for (size_t j = 0; j < k; ++j) {
                if (!used[j] && vs[j] == cur_v) {
                    used[j] = true; ++used_cnt;
                    cur_u = us[j];
                    found = true;
                    break;
                }
            }
        } else {
            // Match on u (stay on side-0 node)
            for (size_t j = 0; j < k; ++j) {
                if (!used[j] && us[j] == cur_u) {
                    used[j] = true; ++used_cnt;
                    cur_v = vs[j];
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            if (err) *err = "Failed to find alternating match in cycle verification";
            return false;
        }
    }

    if (!(cur_u == us[0] && cur_v == vs[0])) {
        if (err) *err = "Cycle does not return to starting node";
        return false;
    }

    if (used_cnt != k) {
        if (err) *err = "Not all edges used in cycle";
        return false;
    }

    return true;
}

} // namespace cuckoo_sip
