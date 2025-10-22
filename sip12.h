#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

// Minimal SipHash-1-2 (1 compression round, 2 finalization rounds)
// Hashes (side, index) with a 128-bit key, returning 64-bit value.
// Caller reduces modulo node-space.

namespace sip12 {

struct Key128 {
    uint64_t k0;
    uint64_t k1;
};

// Parse a 16-byte (32 hex chars) hex string to Key128. If invalid, returns a fixed test key.
Key128 key_from_header_hex(const std::string &hex);

// Hash (side, index) with SipHash-1-2 using 128-bit key.
uint64_t siphash12_u64(uint64_t side, uint64_t idx, const Key128 &key);

}
