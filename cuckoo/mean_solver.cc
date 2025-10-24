#include "mean_solver.h"

#include <unordered_map>
#include <deque>
#include <algorithm>
#include <limits>
#include <thread>
#include <atomic>

namespace cuckoo_sip {

// Portable atomic fetch_or on uint64_t using compiler builtins when available; fallback to non-atomic (single-thread case only uses non-atomic path)
static inline uint64_t atomic_fetch_or_u64(uint64_t* addr, uint64_t mask) {
#if defined(__clang__) || defined(__GNUC__)
    return __atomic_fetch_or(addr, mask, __ATOMIC_RELAXED);
#else
    // In multi-thread path we require atomic; but for non-GCC/Clang toolchains this fallback may not be strictly atomic.
    uint64_t old = *addr; *addr = old | mask; return old;
#endif
}

MeanSolver::MeanSolver(const Params& params, uint32_t threads, uint32_t bucket_bits)
    : p_(params), threads_(threads ? threads : 1), bucket_bits_(bucket_bits) {}

void MeanSolver::init_edge_alive(std::vector<uint64_t>& edge_alive) const {
    const uint64_t N = p_.N;
    const size_t words = words_for_bits(N);
    edge_alive.assign(words, ~0ULL);
    if (words > 0) {
        const uint64_t valid = N & 63ULL;
        if (valid != 0ULL) {
            const uint64_t mask = ((1ULL << valid) - 1ULL);
            edge_alive.back() &= mask;
        }
    }
}

uint64_t MeanSolver::trim_side_bucketed(const std::vector<uint64_t>& edge_alive, std::vector<uint64_t>& new_edge_alive, int side) const {
    const uint64_t N = p_.N;
    const uint64_t bucket_count = (bucket_bits_ >= 32 ? (1ULL << 32) : (1ULL << bucket_bits_));
    const uint64_t bucket_mask = (bucket_bits_ >= 64 ? ~0ULL : ((1ULL << bucket_bits_) - 1ULL));

    // Initialize new edge mask to zeros
    std::fill(new_edge_alive.begin(), new_edge_alive.end(), 0ULL);

    const uint32_t T = threads_ ? threads_ : 1;

    if (T <= 1) {
        // Single-thread original path
        std::vector<std::vector<uint64_t>> buckets;
        buckets.resize(static_cast<size_t>(bucket_count));
        const uint64_t approx_per_bucket = (N / (bucket_count ? bucket_count : 1ULL)) + 1ULL;
        for (auto& b : buckets) b.reserve(static_cast<size_t>(approx_per_bucket));

        // Partition alive edges into buckets by low bits of the chosen side's endpoint
        for (uint64_t i = 0; i < N; ++i) {
            if (!bit_get(edge_alive, i)) continue;
            const node_t x = endpoint(p_, i, side);
            const uint64_t b = static_cast<uint64_t>(x) & bucket_mask;
            buckets[static_cast<size_t>(b)].push_back(i);
        }

        uint64_t kept = 0;
        // For each bucket, count degrees per node and keep edges whose node degree >= 2 on this side.
        for (size_t b = 0; b < buckets.size(); ++b) {
            auto& vec = buckets[b];
            if (vec.empty()) continue;
            std::unordered_map<node_t, uint32_t> deg;
            deg.reserve(vec.size() * 2 + 1);
            for (uint64_t idx : vec) {
                const node_t x = endpoint(p_, idx, side);
                auto it = deg.find(x);
                if (it == deg.end()) deg.emplace(x, 1U); else ++(it->second);
            }
            for (uint64_t idx : vec) {
                const node_t x = endpoint(p_, idx, side);
                if (deg[x] >= 2U) { size_t we = static_cast<size_t>(idx >> 6); uint64_t me = 1ULL << (idx & 63ULL); new_edge_alive[we] |= me; ++kept; }
            }
            std::vector<uint64_t>().swap(vec);
            deg.clear();
        }

        return kept;
    }

    // Multi-threaded path
    // Per-thread buckets to avoid contention
    const uint64_t approx_per_bucket = (N / (bucket_count ? bucket_count : 1ULL)) / T + 1ULL;
    std::vector<std::vector<std::vector<uint64_t>>> thread_buckets;
    thread_buckets.resize(T);
    for (uint32_t t = 0; t < T; ++t) {
        thread_buckets[t].resize(static_cast<size_t>(bucket_count));
        for (auto& v : thread_buckets[t]) v.reserve(static_cast<size_t>(approx_per_bucket));
    }

    const uint64_t chunk = (N + T - 1) / T;
    std::vector<std::thread> ths;
    ths.reserve(T);
    for (uint32_t t = 0; t < T; ++t) {
        uint64_t start = t * chunk;
        uint64_t end = std::min(N, start + chunk);
        if (start >= end) break;
        ths.emplace_back([&, t, start, end]() {
            auto& buckets_local = thread_buckets[t];
            for (uint64_t i = start; i < end; ++i) {
                if (!bit_get(edge_alive, i)) continue;
                const node_t x = endpoint(p_, i, side);
                const uint64_t b = static_cast<uint64_t>(x) & bucket_mask;
                buckets_local[static_cast<size_t>(b)].push_back(i);
            }
        });
    }
    for (auto& th : ths) th.join();

    // Merge thread-local buckets into global buckets
    std::vector<std::vector<uint64_t>> buckets;
    buckets.resize(static_cast<size_t>(bucket_count));
    for (size_t b = 0; b < buckets.size(); ++b) {
        size_t total = 0;
        for (uint32_t t = 0; t < T; ++t) total += thread_buckets[t][b].size();
        if (total == 0) continue;
        buckets[b].reserve(total);
        for (uint32_t t = 0; t < T; ++t) {
            auto& src = thread_buckets[t][b];
            if (!src.empty()) {
                buckets[b].insert(buckets[b].end(), src.begin(), src.end());
                std::vector<uint64_t>().swap(src);
            }
        }
    }

    // Process buckets in parallel; set kept edges in new_edge_alive atomically
    std::atomic<uint64_t> kept_atomic{0};
    ths.clear(); ths.reserve(T);
    const size_t B = buckets.size();
    const size_t bchunk = (B + T - 1) / T;
    for (uint32_t t = 0; t < T; ++t) {
        size_t bstart = static_cast<size_t>(t) * bchunk;
        size_t bend = std::min(B, bstart + bchunk);
        if (bstart >= bend) break;
        ths.emplace_back([&, bstart, bend]() {
            uint64_t kept_local = 0;
            for (size_t b = bstart; b < bend; ++b) {
                auto& vec = buckets[b];
                if (vec.empty()) continue;
                std::unordered_map<node_t, uint32_t> deg;
                deg.reserve(vec.size() * 2 + 1);
                for (uint64_t idx : vec) {
                    const node_t x = endpoint(p_, idx, side);
                    auto it = deg.find(x);
                    if (it == deg.end()) deg.emplace(x, 1U); else ++(it->second);
                }
                for (uint64_t idx : vec) {
                    const node_t x = endpoint(p_, idx, side);
                    if (deg[x] >= 2U) {
                        size_t we = static_cast<size_t>(idx >> 6);
                        uint64_t me = 1ULL << (idx & 63ULL);
                        atomic_fetch_or_u64(&new_edge_alive[we], me);
                        ++kept_local;
                    }
                }
                std::vector<uint64_t>().swap(vec);
                deg.clear();
            }
            kept_atomic.fetch_add(kept_local, std::memory_order_relaxed);
        });
    }
    for (auto& th : ths) th.join();

    return kept_atomic.load(std::memory_order_relaxed);
}

// Sparse DSU + adjacency BFS recovery on the trimmed subgraph for cycle length k.
bool MeanSolver::recover_cycle_k(const std::vector<uint64_t>& edge_alive, uint32_t k, std::vector<uint64_t>& solution) const {
    if (k < 2) return false;
    const uint64_t N = p_.N;
    struct Edge { node_t u; node_t v; uint64_t idx; };
    std::vector<Edge> edges; edges.reserve(1024);
    for (uint64_t i = 0; i < N; ++i) {
        if (!bit_get(edge_alive, i)) continue;
        edges.push_back(Edge{ endpoint(p_, i, 0), endpoint(p_, i, 1), i });
    }
    if (edges.size() < k) return false;

    auto pack = [](int side, node_t n) -> uint64_t { return (static_cast<uint64_t>(side & 1) << 32) | static_cast<uint64_t>(n); };

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
                if (prev.find(y) == prev.end()) { prev[y] = { x, eidx }; dq.push_back(y); }
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

    for (const auto& e : edges) {
        const uint64_t a = pack(0, e.u);
        const uint64_t b = pack(1, e.v);
        const uint64_t ra = dsu.find(a);
        const uint64_t rb = dsu.find(b);
        if (ra == rb) {
            std::vector<uint64_t> path;
            if (bfs_path_edges(a, b, path)) {
                if (path.size() + 1 == k) { solution = path; solution.push_back(e.idx); return true; }
            }
        } else {
            dsu.unite(ra, rb);
            adj[a].push_back({ b, e.idx });
            adj[b].push_back({ a, e.idx });
        }
    }

    return false;
}

MeanResult MeanSolver::solve(uint32_t max_rounds, uint32_t cycle_length) {
    const uint64_t N = p_.N;
    MeanResult res;

    const size_t words_e = words_for_bits(N);
    std::vector<uint64_t> edge_alive; init_edge_alive(edge_alive);
    std::vector<uint64_t> new_edge_alive(words_e, 0ULL);

    uint64_t alive = N;
    for (uint32_t r = 0; r < max_rounds; ++r) {
        // Alternate sides each round
        uint64_t kept0 = trim_side_bucketed(edge_alive, new_edge_alive, 0);
        edge_alive.swap(new_edge_alive);
        uint64_t kept1 = trim_side_bucketed(edge_alive, new_edge_alive, 1);
        edge_alive.swap(new_edge_alive);

        res.rounds_run = r + 1;
        res.alive_edges = kept1;

        if (kept1 == alive) break;
        alive = kept1;
        if (alive == 0) break;
    }

    std::vector<uint64_t> solution;
    if (recover_cycle_k(edge_alive, cycle_length, solution)) {
        res.success = true;
        res.solution_edges = std::move(solution);
        res.note = "Solution found (bucketed DSU/BFS recovery).";
        
    } else {
        res.success = false;
        res.note = "No cycle found in recovery.";
    }

    return res;
}

} // namespace cuckoo_sip
