from cuckoo.api import SolverConfig, solve


def test_solver_runs_small_attempts():
    cfg = SolverConfig(header=b"\x00"*32, n=27, k=2, threads=1, max_attempts=1, time_budget_ms=100)
    res = solve(cfg)
    assert res.found in (True, False)
    assert isinstance(res.metrics, dict)
    assert "hashes_computed" in res.metrics
