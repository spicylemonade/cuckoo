#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "bench/bench.h"

using namespace cuckoo_sip;

static void print_help(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "  --mode {lean,mean}\n"
              << "  --edge-bits N\n"
              << "  --threads T\n"
              << "  --attempts A\n"
              << "  --cycle-length K\n"
              << "  --bucket-bits B            (mean only)\n"
              << "  --hash {sip12,sip24}\n"
              << "  --memcap-bytes-per-edge X   (lean only)\n"
              << "  --header HEX                (optional 16-byte hex for key)\n"
              << "  --baseline                  (run lean sip24 vs sip12 and print ratio)\n"
              << "  --fail-if-slow              (exit nonzero if baseline ratio > 0.5)\n";
}

int main(int argc, char** argv) {
    BenchConfig cfg;
    cfg.mode = "lean";
    cfg.edge_bits = 20;
    cfg.threads = 1;
    cfg.attempts = 3;
    cfg.cycle_length = 42;
    cfg.bucket_bits = 12;
    cfg.variant = SipHashVariant::SipHash12;
    cfg.memcap_bpe = 1.0;

    bool baseline = false;
    bool fail_if_slow = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto need = [&](int more) {
            if (i + more >= argc) {
                std::cerr << "Missing value for " << arg << "\n";
                std::exit(1);
            }
        };
        if (arg == "--mode") { need(1); cfg.mode = argv[++i]; }
        else if (arg == "--edge-bits") { need(1); cfg.edge_bits = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (arg == "--threads") { need(1); cfg.threads = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (arg == "--attempts") { need(1); cfg.attempts = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (arg == "--cycle-length") { need(1); cfg.cycle_length = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (arg == "--bucket-bits") { need(1); cfg.bucket_bits = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (arg == "--hash") { need(1); std::string v = argv[++i]; if (v == "sip12") cfg.variant = SipHashVariant::SipHash12; else if (v == "sip24") cfg.variant = SipHashVariant::SipHash24; else { std::cerr << "Unknown --hash variant: " << v << "\n"; return 1; } }
        else if (arg == "--memcap-bytes-per-edge") { need(1); cfg.memcap_bpe = std::stod(argv[++i]); }
        else if (arg == "--header") { need(1); cfg.header_hex = argv[++i]; }
        else if (arg == "--baseline" || arg == "--baseline-compare") { baseline = true; }
        else if (arg == "--fail-if-slow") { fail_if_slow = true; }
        else if (arg == "--help" || arg == "-h") { print_help(argv[0]); return 0; }
        else { std::cerr << "Unknown option: " << arg << "\n"; print_help(argv[0]); return 1; }
    }

    if (cfg.mode != "lean" && cfg.mode != "mean") { std::cerr << "Invalid --mode: " << cfg.mode << "\n"; return 1; }

    if (cfg.edge_bits >= 32) { std::cerr << "Warning: edge_bits >= 32 may be impractical for this iteration.\n"; }

    if (baseline) {
        if (cfg.mode != "lean") {
            std::cerr << "[info] For --baseline, forcing --mode lean\n";
        }
        bool ok = run_baseline_compare(cfg, fail_if_slow);
        return ok ? 0 : 1;
    }

    auto stats = run_bench(cfg); (void)stats; return 0;
}
