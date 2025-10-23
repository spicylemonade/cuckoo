#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include "cuckoo/graph.h"
#include "cuckoo/lean_solver.h"
#include "cuckoo/mean_solver.h"
#include "verify/verify.h"

namespace benchrun {

struct BenchConfig {
  std::string mode = "lean";
  std::string hash = "sip12";
  uint32_t edge_bits = 20;
  uint32_t attempts = 10;
  uint32_t threads = 1;
  double memcap_bytes_per_edge = 1.0;
  std::string header_hex;
};

struct BenchResult {
  uint32_t attempts = 0;
  uint32_t successes = 0;
  double total_wall_sec = 0.0;
  std::vector<double> success_times_sec;
  double bytes_per_edge = 0.0;
  std::string notes;
};

BenchResult run(const BenchConfig& cfg);

} // namespace benchrun
