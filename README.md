# Cuckoo/Cuckatoo Cycle Linear-Time Memory Tradeoff Solver

This repository provides a Python implementation of a solver searching for 42-cycles in Cuckoo/Cuckatoo graphs while respecting a strict memory budget of N/k bits (peak), where N = 2^n is the number of edges.

Features:
- Configurable memory reduction factor k (k >= 2)
- Partitioned/binned processing to keep memory under N/k bits
- Keyed hash plugin (default: BLAKE2b keyed 64-bit from Python's standard library); pluggable design to swap in SipHash-2-4 later
- Deterministic outputs given header, n, k, threads, seed
- Cycle verification and metrics tracking
- CLI and library API

Install:
- Python >= 3.10
- pip install -e .

Usage (CLI):
- python -m cuckoo.cli --header 00... --n 27 --k 4 --threads 2 --max-attempts 5
- or: cuckoo-solve --header 00... --n 27 --k 4 --threads 2 --time-budget-ms 10000

Library API sketch:
- See cuckoo/api.py and cuckoo/solver.py

Benchmarks:
- See scripts/bench.py and baseline/baseline_lean.csv (placeholder). Fill in with your own machine results.

Testing:
- pytest

License: MIT
