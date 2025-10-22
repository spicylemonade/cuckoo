from cuckoo.hashing import CuckooHash


def test_hash_determinism():
    header = b"\x01" * 32
    h = CuckooHash(header)
    n = 16
    e = 123456
    u1 = h.endpoint(e, 0, n)
    v1 = h.endpoint(e, 1, n)
    u2 = h.endpoint(e, 0, n)
    v2 = h.endpoint(e, 1, n)
    assert u1 == u2 and v1 == v2
    assert 0 <= u1 < (1 << n)
    assert 0 <= v1 < (1 << n)
