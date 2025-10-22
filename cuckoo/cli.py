import argparse
import binascii
from .api import SolverConfig, solve, verify_cycle


def parse_header(hs: str) -> bytes:
    hs = hs.strip().lower().replace("0x", "")
    b = binascii.unhexlify(hs)
    if len(b) != 32:
        raise ValueError("header must be 32 bytes hex")
    return b


def main():
    p = argparse.ArgumentParser(description="Cuckoo/Cuckatoo 42-cycle tradeoff solver (prototype)")
    p.add_argument("header", help="32-byte header hex")
    p.add_argument("n", type=int, choices=[27, 29, 31])
    p.add_argument("k", type=int)
    p.add_argument("threads", type=int, choices=[1, 2, 4, 8])
    p.add_argument("--attempts", type=int, default=1)
    p.add_argument("--time_budget_ms", type=int, default=None)
    args = p.parse_args()

    cfg = SolverConfig(header=parse_header(args.header), n=args.n, k=args.k, threads=args.threads,
                       max_attempts=args.attempts, time_budget_ms=args.time_budget_ms)
    res = solve(cfg)
    print({
        "found": res.found,
        "elapsed_ms": res.elapsed_ms,
        "peak_mem_bytes": res.peak_mem_bytes,
        "metrics": res.metrics,
    })
    if res.found:
        ok = verify_cycle(cfg.header, cfg.n, res.cycle_edges)
        print("verify:", ok)


if __name__ == "__main__":
    main()
