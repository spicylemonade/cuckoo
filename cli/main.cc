#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include "bench/bench.h"

static void print_usage(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [--mode lean|mean] [--hash sip12|sip24] [--edge-bits N] [--threads T] [--attempts A] [--memcap-bytes-per-edge B] [--header-hex 32hex]\n";
}

int main(int argc, char** argv) {
  benchrun::BenchConfig cfg;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto need = [&](int more) {
      if (i + more >= argc) {
        print_usage(argv[0]);
        std::exit(1);
      }
    };
    if (a == "--mode") { need(1); cfg.mode = argv[++i]; }
    else if (a == "--hash") { need(1); cfg.hash = argv[++i]; }
    else if (a == "--edge-bits") { need(1); cfg.edge_bits = (uint32_t)std::stoul(argv[++i]); }
    else if (a == "--threads") { need(1); cfg.threads = (uint32_t)std::stoul(argv[++i]); }
    else if (a == "--attempts") { need(1); cfg.attempts = (uint32_t)std::stoul(argv[++i]); }
    else if (a == "--memcap-bytes-per-edge") { need(1); cfg.memcap_bytes_per_edge = std::stod(argv[++i]); }
    else if (a == "--header-hex") { need(1); cfg.header_hex = argv[++i]; }
    else if (a == "-h" || a == "--help") { print_usage(argv[0]); return 0; }
    else {
      std::cerr << "Unknown option: " << a << "\n";
      print_usage(argv[0]);
      return 1;
    }
  }

  benchrun::BenchResult R = benchrun::run(cfg);

  auto median = [](std::vector<double> v) -> double {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n % 2 == 1) return v[n/2];
    return 0.5 * (v[n/2 - 1] + v[n/2]);
  };

  double med = median(R.success_times_sec);
  double gps = 0.0;
  if (med > 0.0) gps = 1.0 / med;

  std::cout << "Attempts: " << R.attempts << "\n";
  std::cout << "Successes: " << R.successes << "\n";
  std::cout << "Success rate: " << (R.attempts ? (100.0 * R.successes / R.attempts) : 0.0) << "%\n";
  std::cout << "Total wall time (s): " << R.total_wall_sec << "\n";
  std::cout << "Median time/success (s): " << med << "\n";
  std::cout << "gps: " << gps << "\n";
  if (cfg.mode == "lean") {
    std::cout << "Estimated mem bytes/edge (persistent): " << R.bytes_per_edge << "\n";
  }
  if (!R.notes.empty()) std::cout << R.notes << "\n";

  return 0;
}
