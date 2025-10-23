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
};

BenchStats run_bench(const BenchConfig& cfg);

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_BENCH_H
