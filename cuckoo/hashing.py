from __future__ import annotations
from dataclasses import dataclass
import struct
from typing import Protocol
import hashlib

class HashPlugin(Protocol):
    def endpoint(self, edge_index: int, side: int, n: int) -> int: ...


def _u64(x: bytes) -> int:
    return struct.unpack_from("<Q", x)[0]


@dataclass
class Blake2b64:
    key: bytes

    def endpoint(self, edge_index: int, side: int, n: int) -> int:
        # Deterministic keyed 64-bit hash -> map into [0, 2^n)
        # Using BLAKE2b with key; input layout: edge_index (8), side (1)
        h = hashlib.blake2b(digest_size=16, key=self.key)
        h.update(edge_index.to_bytes(8, "little", signed=False))
        h.update(bytes([side & 1]))
        digest = h.digest()
        x = _u64(digest)
        return x & ((1 << n) - 1)


class CuckooHash:
    def __init__(self, header: bytes, impl: str = "blake2b"):
        if impl == "blake2b":
            self.impl: HashPlugin = Blake2b64(header)
        else:
            raise ValueError(f"Unknown hash impl: {impl}")

    def endpoint(self, edge_index: int, side: int, n: int) -> int:
        return self.impl.endpoint(edge_index, side, n)
