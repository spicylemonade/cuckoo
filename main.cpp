#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <thread>
#include <cstdlib>
#include "cuckoo.h"
#include "sip12.h"

struct Args {
    uint32_t n = 20; // default small for quick tests
    uint32_t threads = 1;
    uint32_t attempts = 1;
    std::string header_hex; // empty => default key
    uint32_t rounds = 8;
};

static Args parse_args(int argc, char **argv) {
    Args a;
    for (int i=1;i<argc;i++) {
        std::string s(argv[i]);
        auto eq = s.find('=');
        auto key = s.substr(0, eq);
        std::string val = (eq==std::string::npos?"":s.substr(eq+1));
        if (key == "--n" && !val.empty()) a.n = (uint32_t)std::stoul(val);
        else if (key == "--threads" && !val.empty()) a.threads = (uint32_t)std::stoul(val);
        else if (key == "--attempts" && !val.empty()) a.attempts = (uint32_t)std::stoul(val);
        else if (key == "--header-hex" && !val.empty()) a.header_hex = val;
        else if (key == "--rounds" && !val.empty()) a.rounds = (uint32_t)std::stoul(val);
        else if (key == "-h" || key == "--help") {
            std::cout << "Usage: ./sip12_solver --n=27|29|31 --threads=1|2|4|8 --attempts=N --header-hex=32hex --rounds=R\n";
            std::exit(0);
        }
    }
    return a;
}

int main(int argc, char **argv) {
    auto args = parse_args(argc, argv);
    sip12::Key128 key = sip12::key_from_header_hex(args.header_hex);

    SolverConfig cfg{args.n, args.rounds, args.threads, key};
    CuckooSolver solver(cfg);

    auto t0 = std::chrono::high_resolution_clock::now();
    Proof best{false,{}};
    for (uint32_t a=0;a<args.attempts;a++) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto proof = solver.find_cycle42();
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dt = t2 - t1;
        if (proof.found && proof.edges.size() == 42) {
            bool ok = solver.verify_cycle42(proof.edges);
            if (ok) {
                std::cout << "Attempt " << (a+1) << ": FOUND 42-cycle in " << dt.count() << " s\n";
                std::cout << "Edges:";
                for (auto e: proof.edges) std::cout << ' ' << e;
                std::cout << "\n";
                best = proof;
                break;
            } else {
                std::cout << "Attempt " << (a+1) << ": invalid proof produced (verifier failed) in " << dt.count() << " s\n";
            }
        } else {
            std::cout << "Attempt " << (a+1) << ": NOT FOUND in " << dt.count() << " s\n";
        }
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total = t3 - t0;
    std::cout << "Total time: " << total.count() << " s\n";
    return 0;
}
