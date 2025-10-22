from __future__ import annotations
from dataclasses import dataclass, field
from typing import List, Tuple, Dict, Optional
import platform
import sys
import time
import psutil

from .hashing import CuckooHash

@dataclass
class SolverConfig:
    header: bytes
    n: int  # 27, 29, 31
    k: int  # >= 2
    threads: int  # 1,2,4,8
    seed: int = 0
    max_attempts: int = 1
    time_budget_ms: Optional[int] = None

@dataclass
class SolveResult:
    found: bool
    cycle_edges: List[Tuple[int, int, int]] = field(default_factory=list)
    elapsed_ms: float = 0.0
    peak_mem_bytes: int = 0
    metrics: Dict[str, int | float] = field(default_factory=dict)
    build_info: Dict[str, str] = field(default_factory=dict)


def build_info() -> Dict[str, str]:
    info = {
        "python": sys.version.split()[0],
        "platform": platform.platform(),
        "cpu_count": str(psutil.cpu_count(logical=True)),
    }
    return info


def verify_cycle(header: bytes, n: int, cycle_edges: List[Tuple[int, int, int]]) -> bool:
    """
    Verify a 42-edge directed cycle: list of (edge_index, u, v) tuples.
    Recompute endpoints via hash and ensure adjacency and cycle closure.
    """
    if len(cycle_edges) != 42:
        return False
    h = CuckooHash(header)
    seen_edges = set()
    # Validate endpoints and adjacency
    for i, (eidx, u, v) in enumerate(cycle_edges):
        if eidx in seen_edges:
            return False
        seen_edges.add(eidx)
        uu = h.endpoint(eidx, 0, n)
        vv = h.endpoint(eidx, 1, n)
        if uu != u or vv != v:
            return False
        # adjacency: v of current equals u of next
        nxt = cycle_edges[(i + 1) % 42]
        if v != nxt[1]:
            return False
    # closure ensured by modulo indexing; also ensure distinct nodes could be enforced if needed
    return True


def solve(config: SolverConfig) -> SolveResult:
    """
    Orchestrate the tradeoff solver with attempts and time budget.
    Returns SolveResult with metrics and build info.
    """
    from .solver import TradeoffSolver

    proc = psutil.Process()
    rss_start = getattr(proc.memory_info(), 'rss', 0)
    t0 = time.perf_counter()

    solver = TradeoffSolver(config)

    attempts = 0
    found = False
    cycle: List[Tuple[int, int, int]] = []

    time_budget_ms = config.time_budget_ms if config.time_budget_ms is not None else 0

    while True:
        if config.max_attempts and attempts >= config.max_attempts:
            break
        now_ms = (time.perf_counter() - t0) * 1000.0
        if time_budget_ms and now_ms >= time_budget_ms:
            break

        attempts += 1
        ok, cyc = solver.run_attempt(attempts - 1)
        if ok:
            found = True
            cycle = cyc
            break

    elapsed_ms = (time.perf_counter() - t0) * 1000.0
    rss_end = getattr(proc.memory_info(), 'rss', 0)
    peak = 0
    try:
        peak = proc.memory_info().rss
    except Exception:
        peak = max(rss_start, rss_end)

    metrics = solver.metrics.copy()
    metrics["attempts"] = attempts

    return SolveResult(
        found=found,
        cycle_edges=cycle,
        elapsed_ms=elapsed_ms,
        peak_mem_bytes=peak,
        metrics=metrics,
        build_info=build_info(),
    )
