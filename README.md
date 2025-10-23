Cuckoo Cycle SipHash-1-2 Solver (Iteration 5)

Overview
- Implements both SipHash-1-2 and -2-4 PRFs, graph construction, lean and mean solvers, a k-cycle verifier (42 wrapper), a benchmark harness, and a CLI.
- New in Iteration 5: open-memory (mean) solver with bucketed trimming by low endpoint bits, configurable --bucket-bits flag.

Lean solver (≤1 byte/edge)
- Persistent memory: edge_alive, new_edge_alive, seen/nonleaf for both sides = 0.75 bytes/edge.
- Alternating side-based leaf trimming + DSU/BFS recovery for k-cycles.

Mean solver (open-memory)
- Alternating side-based trimming using radix buckets on low B bits of endpoints (B = --bucket-bits).
- For each side: bucketize alive edges by endpoint low bits, count degrees per node within each bucket, keep edges with degree ≥ 2 on that side.
- After rounds, run DSU/BFS cycle recovery on remaining subgraph.
- Memory is unbounded by design for performance; use smaller EDGE_BITS or B on constrained machines.

Build
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j

Run examples
- Lean, SipHash-1-2:
  cd build && ./cuckoo_sip --mode lean --hash sip12 --edge-bits 20 --threads 1 --attempts 2 --cycle-length 42 --memcap-bytes-per-edge 1

- Mean, SipHash-1-2, 12 radix bits:
  cd build && ./cuckoo_sip --mode mean --hash sip12 --edge-bits 20 --threads 1 --attempts 1 --cycle-length 42 --bucket-bits 12

Notes
- At small edge_bits (e.g., 20), 42-cycles are rare; successes may be 0. Use larger edge_bits (e.g., 27, 29) for realistic mining experiments.
- For the lean bounty attempt, the persistent memory remains ≤ 1 byte/edge; the mean path is unrestricted.
- Future iterations will focus on vectorized SipHash-1-2, multi-threaded passes, fair benchmarking vs Tromp, and a LaTeX paper documenting the design and results.
