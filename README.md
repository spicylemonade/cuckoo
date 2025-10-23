SipHash-1-2 Cuckoo/Cuckatoo Cycle Solver (Lean/Mean scaffolding)

Overview
- Implements a SipHash-keyed bipartite graph per Cuckoo Cycle, with a correct SipHash-1-2 PRF and an optional SipHash-2-4 variant.
- Provides a lean solver (≤1 byte/edge persistent memory) with multi-round trimming and bounded-memory cycle recovery, a mean solver scaffold, a verifier, and a benchmarking CLI.
- Iteration 1 focuses on correctness and infrastructure.

Build
- Requirements: CMake >= 3.13, C++17 compiler

Commands:
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j

Run examples
- Lean, SipHash-1-2, small graph for smoke test:
  cd build && ./cuckoo_sip --mode lean --hash sip12 --edge-bits 20 --threads 1 --attempts 4 --memcap-bytes-per-edge 1

- Mean (placeholder using lean):
  cd build && ./cuckoo_sip --mode mean --hash sip12 --edge-bits 20 --threads 2 --attempts 8

CLI flags
- --mode {lean,mean}
- --hash {sip12,sip24}
- --edge-bits N
- --threads T
- --attempts A
- --memcap-bytes-per-edge X
- --header STR

Design notes
- Graph: endpoints(u,v) computed as siphash_k(2*i + b) mod 2^n with masking.
- SipHash: exact-round implementation with batch helpers.
- Lean solver persistent memory (counts toward ≤1 B/edge):
  - edge_alive bitset: N bits = N/8 bytes.
  - Node degree bitmaps (truncated): per side, two bitmaps (deg>=1, deg>=2) => 2 * 2^n bits; both sides => 4 * 2^n bits = 4N bits = N/2 bytes.
  - Total persistent memory: N/8 + N/2 = 5N/8 bytes = 0.625 bytes/edge.
- Cycle recovery uses a bounded open-addressed parent map sized to fit within remaining budget (if memcap = 1 B/edge).
- Verifier: recomputes endpoints and checks edges form a single 42-cycle with proper alternation and closure.

Caveats (Iteration 1)
- The mean solver is a placeholder.
- Performance is not yet tuned; the goal is a correct baseline.
- Add LICENSE before public release.

