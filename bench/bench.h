#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <random>
#include "cuckoo/graph.h"
#include "cuckoo/lean_solver.h"
#include "cuckoo/mean_solver.h"
#include "verify/verify.h"

namespace cuckoo_sip {

struct BenchConfig {
  std::string mode; // "lean" or "mean"
  std::string hash; // "sip12" or "sip24"
  uint32_t edge_bits{29};
  uint32_t threads{1};
  uint32_t attempts{50};
  double memcap_bpe{1.0};
};

struct BenchResult {
  uint32_t attempts{0};
  uint32_t successes{0};
  double total_time{0.0};
  std::vector<double> success_times;
  double median_time_per_success{0.0};
  double gps{0.0};
  double mem_bytes_per_edge{0.0};
};

BenchResult run_bench(const BenchConfig& cfg, const std::string& header_seed = "");

} // namespace cuckoo_sip
