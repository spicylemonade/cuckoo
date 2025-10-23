Cuckoo/Cuckatoo SipHash-1-2 Solver Skeleton (Iteration 1)

Overview
- Implements a minimal C++ project meeting the repository layout and core functionality:
  - Fast-enough SipHash-1-2 and SipHash-2-4 (scalar) for graph construction.
  - Graph endpoints: u = sip(2*i), v = sip(2*i+1) mod 2^n.
  - Lean solver (≤1 byte/edge) with round-based leaf trimming and streaming cycle recovery.
  - Verifier that checks a list of 42 edge indices forms a single 42-cycle.
  - CLI and simple benchmark harness.
- Mean solver is a stub placeholder in this iteration.

Build
- Requirements: CMake >= 3.12, C++17 compiler (GCC/Clang/MSVC)

Commands:
  mkdir build && cd build
  cmake ..
  make -j

Run examples
- Lean bounty attempt (SipHash-1-2 graph):
  ./cuckoo_sip --mode lean --hash sip12 --edge-bits 20 --threads 1 --attempts 3 --memcap-bytes-per-edge 1

- Try SipHash-2-4 graph construction (for testing):
  ./cuckoo_sip --mode lean --hash sip24 --edge-bits 20 --attempts 3 --memcap-bytes-per-edge 1

- Provide a fixed 16-byte key as header (32 hex chars):
  ./cuckoo_sip --mode lean --hash sip12 --edge-bits 20 --attempts 1 --header-hex 000102030405060708090a0b0c0d0e0f

Notes on correctness and memory
- Graph construction matches the specification: endpoints for edge i are derived via SipHash (selected variant) of nonces 2*i and 2*i+1, masked to 2^edge_bits.
- Verifier recomputes endpoints for the 42 provided edge indices and validates a single 42-cycle by checking degree=2 at each node and closing the loop in 42 steps.

Lean solver design and memory accounting (≤ 1 byte/edge)
- Persistent structures (scale with N = 2^edge_bits):
  - edge_alive[N bits]         : marks which edges survive trimming = N/8 bytes
  - node_degree bits per side  : seen_once[N bits], seen_twice[N bits] on side 0 and side 1 = 4N bits = N/2 bytes
  - Total persistent = N/8 + N/2 = 5N/8 bytes = 0.625 bytes/edge
- Temporary structures: small fixed-size vectors for cycle path (42 entries) and function locals. These do not scale with N.
- Enforcement: the CLI flag --memcap-bytes-per-edge is enforced by the lean solver before allocation/use; if theoretical usage exceeds the cap, the solver aborts that attempt.

Performance caveats (Iteration 1)
- Cycle recovery is implemented via streaming scans to find the unique other edge incident to each node (after trimming). This is correct but slow; it is designed to keep memory usage within cap. Further iterations can add bucketed adjacency summaries or succinct per-node indexing within the cap.
- Single-threaded in this iteration; the threads flag is parsed but not used.

Benchmark protocol scaffolding
- The bench runner executes multiple attempts with randomized keys (or a fixed header if provided), records attempts, successes, and median time/success.
- For bounty comparison (future iteration), build Tromp's reference miners separately and follow the fair benchmarking protocol from the spec.

Project layout
/src
  siphash.h/.cc      - SipHash-1-2 and -2-4 (u64 one-shot), batch helpers
/cuckoo
  graph.h/.cc        - params and endpoint mapping
  lean_solver.h/.cc  - ≤1 byte/edge lean solver
  mean_solver.h/.cc  - stub for future unrestricted-memory solver
/verify
  verify.h/.cc       - 42-cycle validation
/bench
  bench.h/.cc        - simple benchmark runner
/cli
  main.cc            - CLI wrapper

Next steps (future iterations)
- Optimize hashing (batching, SIMD) and trimming passes (cache-friendly partitioning).
- Implement a performant cycle recovery under the 1 byte/edge cap.
- Implement the open-memory mean solver with bucket sorting and adjacency for high throughput.
- Add unit tests (SipHash test vectors, toy graphs) and comprehensive benchmarking/reporting as per the specification.
