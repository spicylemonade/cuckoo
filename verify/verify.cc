#include "verify.h"

#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cuckoo_sip {

bool verify_cycle_k(const Params& p, const std::vector<uint64_t>& edges, size_t k, std::string* err) {
    if (edges.size() != k) {
        if (err) *err = "Edge count does not match cycle length";
        return false;
    }

    // 1) Range + uniqueness
    {
        std::unordered_set<uint64_t> uniq;
        uniq.reserve(k * 2);
        for (auto e : edges) {
            if (e >= p.N) { if (err) *err = "Edge index out of range"; return false; }
            if (!uniq.insert(e).second) { if (err) *err = "Duplicate edge index"; return false; }
        }
    }

    // 2) Endpoints
    std::vector<node_t> us(k), vs(k);
    for (size_t i = 0; i < k; ++i) {
        us[i] = endpoint(p, edges[i], 0);
        vs[i] = endpoint(p, edges[i], 1);
    }

    // 3) Degree == 2 for every participating node on both sides
    std::unordered_map<node_t, int> degU; degU.reserve(k * 2);
    std::unordered_map<node_t, int> degV; degV.reserve(k * 2);
    for (size_t i = 0; i < k; ++i) { degU[us[i]]++; degV[vs[i]]++; }
    for (auto &kv : degU) {
        if (kv.second != 2) {
            if (err) *err = "side-0 node degree != 2";
            return false;
        }
    }
    for (auto &kv : degV) {
        if (kv.second != 2) {
            if (err) *err = "side-1 node degree != 2";
            return false;
        }
    }

    // 4) Traverse one simple cycle of length k that returns to start
    std::unordered_map<node_t, std::pair<int,int>> idxU; idxU.reserve(degU.size() * 2 + 1);
    std::unordered_map<node_t, std::pair<int,int>> idxV; idxV.reserve(degV.size() * 2 + 1);

    auto push2U = [&](node_t n, int i) {
        auto it = idxU.find(n);
        if (it == idxU.end()) it = idxU.emplace(n, std::make_pair(-1, -1)).first;
        auto &pr = it->second;
        if (pr.first == -1) pr.first = i; else pr.second = i;
    };
    auto push2V = [&](node_t n, int i) {
        auto it = idxV.find(n);
        if (it == idxV.end()) it = idxV.emplace(n, std::make_pair(-1, -1)).first;
        auto &pr = it->second;
        if (pr.first == -1) pr.first = i; else pr.second = i;
    };

    for (int i = 0; i < (int)k; ++i) { push2U(us[i], i); push2V(vs[i], i); }

    std::vector<char> used(k, 0);
    int cur = 0; int side = 1; // we used edges[0] first; next we match on side-1 (v)
    used[cur] = 1;

    for (size_t step = 1; step < k; ++step) {
        int nxt = -1;
        if (side == 1) {
            auto pr = idxV[vs[cur]];
            nxt = (pr.first == cur) ? pr.second : pr.first;
        } else {
            auto pr = idxU[us[cur]];
            nxt = (pr.first == cur) ? pr.second : pr.first;
        }
        if (nxt < 0 || used[nxt]) { if (err) *err = "bad alternating traversal"; return false; }
        used[nxt] = 1; cur = nxt; side ^= 1;
    }

    // Must close on the starting edge next
    int nxtClose = -1;
    if (side == 1) {
        auto pr = idxV[vs[cur]]; nxtClose = (pr.first == cur) ? pr.second : pr.first;
    } else {
        auto pr = idxU[us[cur]]; nxtClose = (pr.first == cur) ? pr.second : pr.first;
    }
    if (nxtClose != 0) { if (err) *err = "does not return to start"; return false; }

    return true;
}

} // namespace cuckoo_sip
