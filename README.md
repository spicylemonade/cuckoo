siphash-1-2 Cuckatoo Cycle (42-cycle) CPU solver

Overview
- Plain C++17, no external libs. Implements:
  - sip12: SipHash-1-2 (1 compression, 2 finalization rounds) for endpoint generation
  - cuckoo: trimming solver and cycle finder
  - main: CLI + verifier + simple benchmarking

Status
- Baseline functional solver with trimming and DFS cycle search (single-threaded).
- Targets correctness and scaffolding; further iterations will add threading, binning, SIMD, and benchmarking to aim at 2x Tromp baseline.

Build
- g++ -O3 -march=native -std=c++17 -pthread main.cpp cuckoo.cpp sip12.cpp -o sip12_solver

CLI
- ./sip12_solver --n=20 --threads=1 --attempts=1 --header-hex=000102030405060708090a0b0c0d0e0f
- Options:
  - --n: graph size exponent (N = 2^n edges). For testing start with n=20..24.
  - --threads: number of threads (current baseline uses 1).
  - --attempts: number of solver attempts (each with fresh trimming & search).
  - --header-hex: 16-byte hex to derive 128-bit sip key. If omitted, uses a fixed test key.
  - --rounds: number of trimming rounds.

Output
- Prints FOUND 42-cycle and ascending edge indices when found, otherwise NOT FOUND.
- Includes basic timing metrics.

Design
- Bipartite graph with N+N nodes and N edges.
- Endpoints u = endpoint(0,i), v = endpoint(1,i) via SipHash-1-2 keyed on (side,i), reduced modulo nodes_per_side.
- Trimming: repeated removal of edges incident to degree-1 nodes to shrink subgraph before DFS.
- Cycle finding: DFS on reduced subgraph to extract cycles; verify length==42.

Files
- sip12.h/.cpp: compact SipHash-1-2
- cuckoo.h/.cpp: solver core
- main.cpp: CLI + verifier

Notes
- This is a scaffold optimized for clarity and future optimization: binning, cache-friendly layouts, multithreading.
- A separate verify_cycle42 function recomputes endpoints to check the walk closes in 42 steps.
