#include "lean_solver.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <stdexcept>
#include <deque>
#include <thread>
#include <mutex>

namespace cuckoo_sip {

// Portable atomic fetch_or on uint64_t using compiler builtins when available; fallback to mutex.
static inline uint64_t atomic_fetch_or_u64(uint64_t* addr, uint64_t mask) {
#if defined(__clang__) || defined(__GNUC__)
    return __atomic_fetch_or(addr, mask, __ATOMIC_RELAXED);
#else
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    uint64_t old = *addr;
    *addr = old | mask;
    return old;
#endif
}

LeanSolver::LeanSolver(const Params& params, uint32_t threads, double memcap_bytes_per_edge)
    : p_(params), threads_(threads ? threads : 1), memcap_bpe_(memcap_bytes_per_edge) {
    // Enforce memory cap (persistent bitsets only)
    if (mem_bytes_per_edge() > memcap_bpe_) {
        throw std::runtime_error("Lean solver theoretical mem exceeds cap: " +
                                 std::to_string(mem_bytes_per_edge()) + ">" + std::to_string(memcap_bpe_));
    }
}

size_t LeanSolver::memory_usage_bytes() const {
    const uint64_t N = p_.N;
    const size_t words = words_for_bits(N);
    // Persistent bitsets: edge_alive (N bits), new_edge_alive (N bits),
    // seen0, nonleaf0, seen1, nonleaf1 (each N bits)
    const size_t bytes = (2 + 4) * words * sizeof(uint64_t);
    return bytes;
}

double LeanSolver::mem_bytes_per_edge() const {
    if (p_.N == 0) return 0.0;
    return static_cast<double>(memory_usage_bytes()) / static_cast<double>(p_.N);
}

void LeanSolver::init_edge_alive(std::vector<uint64_t>& edge_alive) const {
    const uint64_t N = p_.N;
    const size_t words = words_for_bits(N);
    edge_alive.assign(words, ~0ULL);
    // Mask off extra bits in last word
    if (words > 0) {
        const uint64_t valid = N & 63ULL;
        if (valid != 0ULL) {
            const uint64_t mask = ((1ULL << valid) - 1ULL);
            edge_alive.back() &= mask;
        }
    }
}

uint64_t LeanSolver::trim_round_both(const std::vector<uint64_t>& edge_alive,
                                      std::vector<uint64_t>& new_edge_alive,
                                      std::vector<uint64_t>& seen0,
                                      std::vector<uint64_t>& nonleaf0,
                                      std::vector<uint64_t>& seen1,
                                      std::vector<uint64_t>& nonleaf1) const {
    const uint64_t N = p_.N;

    // Reset bitmaps
    std::fill(seen0.begin(), seen0.end(), 0ULL);
    std::fill(nonleaf0.begin(), nonleaf0.end(), 0ULL);
    std::fill(seen1.begin(), seen1.end(), 0ULL);
    std::fill(nonleaf1.begin(), nonleaf1.end(), 0ULL);

    const uint32_t T = threads_ ? threads_ : 1;
    const uint64_t chunk = (N + T - 1) / T;

    // Pass 1: build seen and nonleaf bitmaps for both sides using atomic fetch_or on bitset words
    std::vector<std::thread> ths;
    ths.reserve(T);
    for (uint32_t t = 0; t < T; ++t) {
        uint64_t start = t * chunk;
        uint64_t end = std::min(N, start + chunk);
        if (start >= end) break;
        ths.emplace_back([&, start, end]() {
            for (uint64_t i = start; i < end; ++i) {
                if (!bit_get(edge_alive, i)) continue;
                const node_t u = endpoint(p_, i, 0);
                const node_t v = endpoint(p_, i, 1);
                size_t wu = static_cast<size_t>(u >> 6);
                uint64_t mu = 1ULL << (u & 63ULL);
                uint64_t oldu = atomic_fetch_or_u64(&seen0[wu], mu);
                if (oldu & mu) { atomic_fetch_or_u64(&nonleaf0[wu], mu); }

                size_t wv = static_cast<size_t>(v >> 6);
                uint64_t mv = 1ULL << (v & 63ULL);
                uint64_t oldv = atomic_fetch_or_u64(&seen1[wv], mv);
                if (oldv & mv) { atomic_fetch_or_u64(&nonleaf1[wv], mv); }
            }
        });
    }
    for (auto& th : ths) th.join();

    // Pass 2: keep edges with both endpoints in nonleaf
    std::fill(new_edge_alive.begin(), new_edge_alive.end(), 0ULL);
    std::vector<uint64_t> kept_local(T, 0ULL);
    ths.clear(); ths.reserve(T);
    for (uint32_t t = 0; t < T; ++t) {
        uint64_t start = t * chunk;
        uint64_t end = std::min(N, start + chunk);
        if (start >= end) break;
        ths.emplace_back([&, start, end, t]() {
            uint64_t kept = 0;
            for (uint64_t i = start; i < end; ++i) {
                if (!bit_get(edge_alive, i)) continue;
                const node_t u = endpoint(p_, i, 0);
                const node_t v = endpoint(p_, i, 1);
                size_t wu = static_cast<size_t>(u >> 6);
                uint64_t mu = 1ULL << (u & 63ULL);
                size_t wv = static_cast<size_t>(v >> 6);
                uint64_t mv = 1ULL << (v & 63ULL);
                if ((seen0[wu] & mu) && (nonleaf0[wu] & mu) && (seen1[wv] & mv) && (nonleaf1[wv] & mv)) {
                    size_t we = static_cast<size_t>(i >> 6);
                    uint64_t me = 1ULL << (i & 63ULL);
                    atomic_fetch_or_u64(&new_edge_alive[we], me);
                    ++kept;
                }
            }
            kept_local[t] = kept;
        });
    }
    for (auto& th : ths) th.join();

    uint64_t kept = 0; for (auto x : kept_local) kept += x;
    return kept;
}

uint64_t LeanSolver::trim_round_side(const std::vector<uint64_t>& edge_alive,
                                     std::vector<uint64_t>& new_edge_alive,
                                     std::vector<uint64_t>& seen_side,
                                     std::vector<uint64_t>& nonleaf_side,
                                     int side) const {
    const uint64_t N = p_.N;

    std::fill(seen_side.begin(), seen_side.end(), 0ULL);
    std::fill(nonleaf_side.begin(), nonleaf_side.end(), 0ULL);

    const uint32_t T = threads_ ? threads_ : 1;
    const uint64_t chunk = (N + T - 1) / T;

    // Build seen/nonleaf for the chosen side using atomic fetch_or on words
    std::vector<std::thread> ths;
    ths.reserve(T);
    for (uint32_t t = 0; t < T; ++t) {
        uint64_t start = t * chunk;
        uint64_t end = std::min(N, start + chunk);
        if (start >= end) break;
        ths.emplace_back([&, start, end]() {
            for (uint64_t i = start; i < end; ++i) {
                if (!bit_get(edge_alive, i)) continue;
                const node_t x = endpoint(p_, i, side);
                size_t wx = static_cast<size_t>(x >> 6);
                uint64_t mx = 1ULL << (x & 63ULL);
                uint64_t oldx = atomic_fetch_or_u64(&seen_side[wx], mx);
                if (oldx & mx) { atomic_fetch_or_u64(&nonleaf_side[wx], mx); }
            }
        });
    }
    for (auto& th : ths) th.join();

    // Keep edges whose chosen endpoint is nonleaf
    std::fill(new_edge_alive.begin(), new_edge_alive.end(), 0ULL);
    std::vector<uint64_t> kept_local(T, 0ULL);
    ths.clear(); ths.reserve(T);
    for (uint32_t t = 0; t < T; ++t) {
        uint64_t start = t * chunk;
        uint64_t end = std::min(N, start + chunk);
        if (start >= end) break;
        ths.emplace_back([&, start, end, t]() {
            uint64_t kept = 0;
            for (uint64_t i = start; i < end; ++i) {
                if (!bit_get(edge_alive, i)) continue;
                const node_t x = endpoint(p_, i, side);
                size_t wx = static_cast<size_t>(x >> 6);
                uint64_t mx = 1ULL << (x & 63ULL);
                if ((seen_side[wx] & mx) && (nonleaf_side[wx] & mx)) {
                    size_t we = static_cast<size_t>(i >> 6);
                    uint64_t me = 1ULL << (i & 63ULL);
                    atomic_fetch_or_u64(&new_edge_alive[we], me);
                    ++kept;
                }
            }
            kept_local[t] = kept;
        });
    }
    for (auto& th : ths) th.join();

    uint64_t kept = 0; for (auto x : kept_local) kept += x;
    return kept;
}

// Sparse DSU + adjacency BFS recovery on the trimmed subgraph for cycle length k.
bool LeanSolver::recover_cycle_k(const std::vector<uint64_t>& edge_alive, uint32_t k, std::vector<uint64_t>& solution) const {
    if (k < 2) return false;
    const uint64_t N = p_.N;
    struct Edge { node_t u; node_t v; uint64_t idx; };
    std::vector<Edge> edges; edges.reserve(1024);
    for (uint64_t i = 0; i < N; ++i) {
        if (!bit_get(edge_alive, i)) continue;
        edges.push_back(Edge{ endpoint(p_, i, 0), endpoint(p_, i, 1), i });
    }
    if (edges.size() < k) return false;

    auto pack = [](int side, node_t n) -> uint64_t {
        return (static_cast<uint64_t>(side & 1) << 32) | static_cast<uint64_t>(n);
    };

    struct DSU {
        std::unordered_map<uint64_t, uint64_t> p;
        std::unordered_map<uint64_t, uint32_t> sz;
        uint64_t find(uint64_t x) {
            auto it = p.find(x);
            if (it == p.end()) { p[x] = x; sz[x] = 1; return x; }
            if (it->second == x) return x;
            uint64_t r = find(it->second);
            p[x] = r;
            return r;
        }
        bool unite(uint64_t a, uint64_t b) {
            a = find(a); b = find(b);
            if (a == b) return false;
            uint32_t sa = sz[a], sb = sz[b];
            if (sa < sb) { std::swap(a, b); std::swap(sa, sb); }
            p[b] = a;
            sz[a] = sa + sb;
            return true;
        }
    } dsu;

    // adjacency forest for edges that joined components so far
    std::unordered_map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> adj;
    adj.reserve(edges.size() * 2 + 1);

    auto bfs_path_edges = [&](uint64_t src, uint64_t dst, std::vector<uint64_t>& path_out) -> bool {
        std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> prev; // node -> (parent, edge_idx)
        std::deque<uint64_t> dq;
        dq.push_back(src);
        prev[src] = { std::numeric_limits<uint64_t>::max(), 0ULL };
        bool found = false;
        while (!dq.empty()) {
            uint64_t x = dq.front(); dq.pop_front();
            if (x == dst) { found = true; break; }
            auto it = adj.find(x);
            if (it == adj.end()) continue;
            for (const auto& pr : it->second) {
                uint64_t y = pr.first; uint64_t eidx = pr.second;
                if (prev.find(y) == prev.end()) {
                    prev[y] = { x, eidx };
                    dq.push_back(y);
                }
            }
        }
        if (!found) return false;
        // reconstruct edge list from dst back to src
        path_out.clear();
        uint64_t cur = dst;
        while (true) {
            auto it = prev.find(cur);
            if (it == prev.end()) return false;
            if (it->second.first == std::numeric_limits<uint64_t>::max()) break;
            path_out.push_back(it->second.second);
            cur = it->second.first;
        }
        std::reverse(path_out.begin(), path_out.end());
        return true;
    };

    // Iterate edges; when an edge links nodes already connected, extract the unique path and test length k-1.
    for (const auto& e : edges) {
        const uint64_t a = pack(0, e.u);
        const uint64_t b = pack(1, e.v);
        const uint64_t ra = dsu.find(a);
        const uint64_t rb = dsu.find(b);
        if (ra == rb) {
            std::vector<uint64_t> path;
            if (bfs_path_edges(a, b, path)) {
                if (path.size() + 1 == k) {
                    solution = path;
                    solution.push_back(e.idx);
                    return true;
                }
            }
        } else {
            dsu.unite(ra, rb);
            adj[a].push_back({ b, e.idx });
            adj[b].push_back({ a, e.idx });
        }
    }

    return false;
}

LeanResult LeanSolver::solve(uint32_t max_rounds, uint32_t cycle_length) {
    const uint64_t N = p_.N;
    LeanResult res;
    res.mem_bytes_per_edge = mem_bytes_per_edge();

    // Allocate bitsets
    const size_t words_e = words_for_bits(N);
    std::vector<uint64_t> edge_alive; init_edge_alive(edge_alive);
    std::vector<uint64_t> new_edge_alive(words_e, 0ULL);
    std::vector<uint64_t> seen0(words_e, 0ULL), nonleaf0(words_e, 0ULL);
    std::vector<uint64_t> seen1(words_e, 0ULL), nonleaf1(words_e, 0ULL);

    uint64_t alive = N;
    for (uint32_t r = 0; r < max_rounds; ++r) {
        // Alternate-side trimming within each round for better convergence
        uint64_t kept0 = trim_round_side(edge_alive, new_edge_alive, seen0, nonleaf0, 0);
        edge_alive.swap(new_edge_alive);
        uint64_t kept1 = trim_round_side(edge_alive, new_edge_alive, seen1, nonleaf1, 1);
        edge_alive.swap(new_edge_alive);

        res.rounds_run = r + 1;
        res.alive_edges = kept1;

        if (kept1 == alive) {
            // As a final check, do combined trimming to remove any remaining leaves on both sides simultaneously
            uint64_t kept2 = trim_round_both(edge_alive, new_edge_alive, seen0, nonleaf0, seen1, nonleaf1);
            edge_alive.swap(new_edge_alive);
            res.alive_edges = kept2;
            if (kept2 == alive) break;
            alive = kept2;
        } else {
            alive = kept1;
        }

        if (alive == 0) break;
    }

    // Try to recover a k-cycle from remaining subgraph
    std::vector<uint64_t> solution;
    if (recover_cycle_k(edge_alive, cycle_length, solution)) {
        res.success = true;
        res.solution_edges = std::move(solution);
        res.note = "Solution found (DSU/BFS recovery).";
    } else {
        res.success = false;
        res.note = "No cycle found in recovery.";
    }

    return res;
}

} // namespace cuckoo_sip
