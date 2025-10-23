#include "bench/bench.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <functional>

namespace cuckoo_sip {

static Key128 key_from_header_and_attempt(const std::string& header, uint64_t attempt_idx) {
  std::hash<std::string> H;
  uint64_t h0 = H(header);
  uint64_t h1 = H(header + "#sip");
  h0 ^= (attempt_idx * 0x9e3779b97f4a7c15ULL);
  h1 ^= (attempt_idx * 0xC2B2AE3D27D4EB4FULL);
  return Key128{h0, h1};
}

static SipVariant parse_hash(const std::string& s) {
  if (s == "sip24") return SipVariant::SIPHASH_24;
  return SipVariant::SIPHASH_12;
}

BenchResult run_bench(const BenchConfig& cfg, const std::string& header_seed) {
  BenchResult br;
  br.attempts = cfg.attempts;

  std::mutex mtx;

  auto worker = [&](uint32_t tid, uint32_t start, uint32_t end) {
    for (uint32_t i = start; i < end; ++i) {
      Params p;
      p.edge_bits = cfg.edge_bits;
      p.N = 1ULL << cfg.edge_bits;
      p.key = key_from_header_and_attempt(header_seed, i);
      p.hash_variant = parse_hash(cfg.hash);
      Graph G(p);

      auto t0 = std::chrono::steady_clock::now();
      bool found = false;
      std::vector<uint32_t> solution;
      double mem_bpe = 0.0;

      if (cfg.mode == "lean") {
        LeanSolver solver(G, cfg.memcap_bpe, /*trim_rounds*/ 8);
        auto res = solver.solve_42_cycle();
        found = res.found;
        solution = res.cycle_edges;
        mem_bpe = res.stats.mem_bytes_per_edge;
      } else {
        MeanSolver solver(G);
        auto res = solver.solve_42_cycle();
        found = res.found;
        solution = res.cycle_edges;
        mem_bpe = 0.0;
      }

      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();

      if (found) {
        auto vr = verify_42_cycle(G, solution);
        if (!vr.ok) {
          found = false;
        }
      }

      std::lock_guard<std::mutex> lock(mtx);
      br.total_time += secs;
      if (found) {
        br.successes++;
        br.success_times.push_back(secs);
        if (mem_bpe > br.mem_bytes_per_edge) br.mem_bytes_per_edge = mem_bpe;
      }
    }
  };

  uint32_t T = std::max(1u, cfg.threads);
  uint32_t per = (cfg.attempts + T - 1) / T;
  std::vector<std::thread> threads;
  uint32_t idx = 0;
  for (uint32_t t = 0; t < T; ++t) {
    uint32_t start = idx;
    uint32_t end = std::min(cfg.attempts, idx + per);
    if (start >= end) break;
    threads.emplace_back(worker, t, start, end);
    idx = end;
  }
  for (auto& th : threads) th.join();

  if (!br.success_times.empty()) {
    std::sort(br.success_times.begin(), br.success_times.end());
    br.median_time_per_success = br.success_times[br.success_times.size()/2];
    double total_succ_time = 0.0;
    for (double s : br.success_times) total_succ_time += s;
    double mean_time = total_succ_time / (double)br.success_times.size();
    br.gps = (mean_time > 0 ? 1.0 / mean_time : 0.0);
  }

  return br;
}

} // namespace cuckoo_sip
