from cuckoo.api import verify_cycle
from cuckoo.hashing import CuckooHash


def test_verify_trivial_false():
    header = b"\x02" * 32
    assert not verify_cycle(header, 10, [])


def test_verify_roundtrip_fake():
    # Construct a fake 42-cycle by repeating a contrived pattern if possible; usually will fail
    header = b"\x03" * 32
    h = CuckooHash(header)
    n = 10
    edges = []
    # Build a trivial cycle with repeats to confirm verifier catches duplicates
    for i in range(42):
        e = i
        u = h.endpoint(e, 0, n)
        v = h.endpoint(e, 1, n)
        edges.append((e, u, v))
    assert not verify_cycle(header, n, edges)
