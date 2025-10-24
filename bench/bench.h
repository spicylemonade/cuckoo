#ifndef CUCKOO_SIP_BENCH_H
#define CUCKOO_SIP_BENCH_H

#include <cstdint>
#include <vector>
#include <string>

#include "cuckoo/graph.h"

namespace cuckoo_sip {

struct BenchConfig {
    std::string mode;         // "lean" or "mean"
    uint32_t edge_bits = 29;
    uint32_t threads = 1;
    uint32_t attempts = 10;
    uint32_t cycle_length = 42;
    uint32_t bucket_bits = 12; // mean solver bucket radix bits
    SipHashVariant variant = SipHashVariant::SipHash12;
    double memcap_bpe = 1.0;  // used in lean mode only
    std::string header_hex;   // optional fixed key from hex; if empty, random per attempt
};

struct BenchStats {
    uint32_t attempts = 0;
    uint32_t successes = 0;
    double total_wall_s = 0.0;
    double median_time_success_s = 0.0;
    double geomean_time_success_s = 0.0;
    double gps = 0.0; // graphs per second per success measure
    double mem_bpe = 0.0; // lean solver theoretical mem per edge
    std::vector<double> times_success_s;
    // New: aggregate times for all attempts
    std::vector<double> times_all_s;
    double median_time_all_s = 0.0;
};

BenchStats run_bench(const BenchConfig& cfg);

// Baseline comparison harness: runs lean solver with sip24 (baseline) and sip12 on same parameters
// Prints the ratio = median_time_all_s(sip12) / median_time_all_s(sip24). Returns true if ratio <= 0.5.
bool run_baseline_compare(const BenchConfig& base_cfg, bool exit_if_ratio_exceeds = false);

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_BENCH_H
