from __future__ import annotations
import argparse
import csv
import time
from pathlib import Path
from cuckoo.api import SolverConfig, solve


def load_baseline(path: Path):
    tbl = {}
    with path.open() as f:
        rd = csv.DictReader(f)
        for row in rd:
            tbl[(int(row["n"]), int(row["threads"]))] = float(row["T_lean_ms"])
    return tbl


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--header", required=False, default="00"*32)
    p.add_argument("--n", type=int, choices=[27,29,31], required=True)
    p.add_argument("--k", type=int, default=2)
    p.add_argument("--threads", type=int, choices=[1,2,4,8], default=1)
    p.add_argument("--attempts", type=int, default=3)
    p.add_argument("--baseline", type=str, default=str(Path(__file__).resolve().parent.parent / "baseline" / "baseline_lean.csv"))
    args = p.parse_args()

    baseline = load_baseline(Path(args.baseline))
    cfg = SolverConfig(header=bytes.fromhex(args.header), n=args.n, k=args.k, threads=args.threads, max_attempts=args.attempts)
    res = solve(cfg)
    key = (args.n, args.threads)
    tlean = baseline.get(key, None)
    if tlean is not None:
        allowed = 10 * args.k * tlean
        print(f"Elapsed {res.elapsed_ms:.1f} ms; Allowed <= {allowed:.1f} ms vs baseline {tlean} ms")
    else:
        print(f"Elapsed {res.elapsed_ms:.1f} ms (no baseline)")
    print(res)

if __name__ == "__main__":
    main()
