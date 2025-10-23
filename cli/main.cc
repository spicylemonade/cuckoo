#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "bench/bench.h"

using namespace cuckoo_sip;

static void usage(const char* prog) {
  std::cerr << "Usage: " << prog << " --mode {lean|mean} --hash {sip12|sip24} --edge-bits N --threads T --attempts A [--memcap-bytes-per-edge X] [--header STR]\n";
}

int main(int argc, char** argv) {
  BenchConfig cfg;
  cfg.mode = "lean";
  cfg.hash = "sip12";
  cfg.edge_bits = 20; // smaller default for smoke test
  cfg.threads = 1;
  cfg.attempts = 4;
  cfg.memcap_bpe = 1.0;
  std::string header = "cuckoo_sip_header";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next = [&](int& i) -> std::string { if (i+1 < argc) return std::string(argv[++i]); usage(argv[0]); std::exit(1); };
    if (arg == "--mode") cfg.mode = next(i);
    else if (arg == "--hash") cfg.hash = next(i);
    else if (arg == "--edge-bits") cfg.edge_bits = (uint32_t)std::stoul(next(i));
    else if (arg == "--threads") cfg.threads = (uint32_t)std::stoul(next(i));
    else if (arg == "--attempts") cfg.attempts = (uint32_t)std::stoul(next(i));
    else if (arg == "--memcap-bytes-per-edge") cfg.memcap_bpe = std::stod(next(i));
    else if (arg == "--header") header = next(i);
    else if (arg == "-h" || arg == "--help") { usage(argv[0]); return 0; }
    else { std::cerr << "Unknown flag: " << arg << "\n"; usage(argv[0]); return 1; }
  }

  if (cfg.mode != "lean" && cfg.mode != "mean") { std::cerr << "Invalid --mode\n"; return 1; }
  if (cfg.hash != "sip12" && cfg.hash != "sip24") { std::cerr << "Invalid --hash\n"; return 1; }
  if (cfg.edge_bits == 0 || cfg.edge_bits > 31) { std::cerr << "--edge-bits must be in [1,31]\n"; return 1; }

  auto br = run_bench(cfg, header);

  std::cout << "Attempts: " << br.attempts << "\n";
  std::cout << "Successes: " << br.successes << "\n";
  std::cout << "Total time (s): " << br.total_time << "\n";
  std::cout << "Median time/success (s): " << br.median_time_per_success << "\n";
  std::cout << "GPS (graphs/success per second): " << br.gps << "\n";
  if (cfg.mode == "lean") std::cout << "Mem bytes/edge (theoretical persistent): " << br.mem_bytes_per_edge << "\n";
  std::cout << std::flush;
  return 0;
}
