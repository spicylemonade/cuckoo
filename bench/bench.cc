#include "bench.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#include "util.h"
#include "cuckoo/lean_solver.h"
#include "cuckoo/mean_solver.h"
#include "verify/verify.h"

namespace cuckoo_sip {

static inline double ns_to_s(uint64_t ns) { return double(ns) * 1e-9; }

BenchStats run_bench(const BenchConfig& cfg) {
    BenchStats stats;
    stats.attempts = cfg.attempts;

    for (uint32_t a = 0; a < cfg.attempts; ++a) {
        Params p;
        set_edge_bits(p, cfg.edge_bits);

        // Key derivation: use fixed header_hex if provided
        std::string header;
        if (!cfg.header_hex.empty()) {
            header = cfg.header_hex;
            auto parsed = parse_hex_key128(cfg.header_hex);
            if (parsed) p.key = *parsed; else p.key = derive_key_from_header(header);
        } else {
            header = random_hex_header();
            p.key = derive_key_from_header(header);
        }
        p.variant = cfg.variant;

        uint64_t t0 = now_ns();
        bool success = false;
        std::vector<uint64_t> solution;
        double mem_bpe = 0.0;
        std::string note;

        try {
            if (cfg.mode == "lean") {
                LeanSolver solver(p, cfg.threads, cfg.memcap_bpe);
                auto res = solver.solve(256, cfg.cycle_length);
                success = res.success;
                solution = std::move(res.solution_edges);
                mem_bpe = res.mem_bytes_per_edge;
                note = res.note;
            } else if (cfg.mode == "mean") {
                MeanSolver solver(p, cfg.threads, cfg.bucket_bits);
                auto res = solver.solve(8, cfg.cycle_length);
                success = res.success;
                solution = std::move(res.solution_edges);
                note = res.note;
            } else {
                throw std::runtime_error("Unknown mode: " + cfg.mode);
            }
        } catch (const std::exception& e) {
            success = false;
            note = std::string("Exception: ") + e.what();
        }

        uint64_t t1 = now_ns();
        double dt = ns_to_s(t1 - t0);
        stats.total_wall_s += dt;

        if (success) {
            // Verify solution (supports arbitrary k)
            std::string err;
            if (verify_cycle_k(p, solution, cfg.cycle_length, &err)) {
                stats.successes++;
                stats.times_success_s.push_back(dt);
                std::cout << "Solution edges (" << cfg.cycle_length << "): ";
                for (size_t k = 0; k < solution.size(); ++k) { if (k) std::cout << ","; std::cout << solution[k]; }
                std::cout << "\n";
            } else {
                std::cerr << "Verification failed: " << err << "\n";
                success = false;
            }
        }

        if (a == 0 && cfg.mode == "lean") stats.mem_bpe = mem_bpe;

        std::cout << std::fixed << std::setprecision(6)
                  << "Attempt " << (a + 1) << "/" << cfg.attempts
                  << ", header= " << header
                  << ", success= " << (success ? "yes" : "no")
                  << ", time_s= " << dt
                  << (note.empty() ? "" : (std::string(", note= ") + note))
                  << "\n";
    }

    if (!stats.times_success_s.empty()) {
        auto v = stats.times_success_s;
        std::sort(v.begin(), v.end());
        stats.median_time_success_s = (v.size() % 2 == 1) ? v[v.size()/2] : 0.5 * (v[v.size()/2 - 1] + v[v.size()/2]);
        double sumlog = 0.0;
        for (double x : stats.times_success_s) sumlog += std::log(std::max(1e-12, x));
        stats.geomean_time_success_s = std::exp(sumlog / stats.times_success_s.size());
        stats.gps = 1.0 / stats.median_time_success_s;
    } else {
        stats.median_time_success_s = 0.0;
        stats.geomean_time_success_s = 0.0;
        stats.gps = 0.0;
    }

    std::cout << "\nSummary:\n";
    std::cout << "  mode           : " << cfg.mode << "\n";
    std::cout << "  hash variant   : " << (cfg.variant == SipHashVariant::SipHash12 ? "sip12" : "sip24") << "\n";
    std::cout << "  edge_bits      : " << cfg.edge_bits << "\n";
    std::cout << "  attempts       : " << cfg.attempts << "\n";
    if (cfg.mode == "mean") { std::cout << "  bucket_bits    : " << cfg.bucket_bits << "\n"; }
    std::cout << "  successes      : " << stats.successes << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  total_wall_s   : " << stats.total_wall_s << "\n";
    std::cout << "  median_t/succ  : " << stats.median_time_success_s << "\n";
    std::cout << "  geomean_t/succ : " << stats.geomean_time_success_s << "\n";
    std::cout << "  gps            : " << stats.gps << "\n";
    if (cfg.mode == "lean") { std::cout << "  mem bytes/edge : " << stats.mem_bpe << "\n"; }

    return stats;
}

} // namespace cuckoo_sip
