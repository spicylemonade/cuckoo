#include <cassert>
#include <iostream>
#include <vector>
#include "cuckoo.h"
#include "sip12.h"

int main() {
    // Basic verify tests
    sip12::Key128 key{0x0706050403020100ULL, 0x0f0e0d0c0b0a0908ULL};
    SolverConfig cfg{16, 4, 1, key};
    CuckooSolver solver(cfg);

    // Negative: wrong size
    std::vector<uint64_t> bad_size(41, 0);
    assert(!solver.verify_cycle42(bad_size));

    // Negative: out of order / duplicates
    std::vector<uint64_t> bad_dups = {1,1,2};
    assert(!solver.verify_cycle42(bad_dups));

    // Negative: out of range value
    std::vector<uint64_t> bad_range(42);
    for (int i=0;i<42;i++) bad_range[i] = (1u<<16) - 42 + i;
    assert(!solver.verify_cycle42(bad_range));

    // Run a small attempt; likely NOT FOUND for this small n, but verifier shouldn't crash
    auto proof = solver.find_cycle42();
    if (proof.found) {
        // Verify must accept returned proof
        bool ok = solver.verify_cycle42(proof.edges);
        assert(ok);
    }

    std::cout << "All tests passed (basic verifier and integration).\n";
    return 0;
}
