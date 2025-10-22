#include "sip12.h"
#include <cstring>
#include <string>
#include <cctype>

namespace sip12 {

static inline uint64_t rotl(uint64_t x, int b) {
    return (x << b) | (x >> (64 - b));
}

// The SipHash round: same as reference; we'll run 1 compression round, 2 finalization rounds.
static inline void sip_round(uint64_t &v0, uint64_t &v1, uint64_t &v2, uint64_t &v3) {
    v0 += v1; v2 += v3; v1 = rotl(v1, 13); v3 = rotl(v3, 16); v1 ^= v0; v3 ^= v2; v0 = rotl(v0, 32);
    v2 += v1; v0 += v3; v1 = rotl(v1, 17); v3 = rotl(v3, 21); v1 ^= v2; v3 ^= v0; v2 = rotl(v2, 32);
}

// Pack (side, idx) into 16 bytes (128 bits) message: 8 bytes idx, 8 bytes side
static inline void pack_message(uint64_t side, uint64_t idx, uint8_t out[16]) {
    for (int i=0;i<8;i++) out[i] = (uint8_t)((idx >> (8*i)) & 0xff);
    for (int i=0;i<8;i++) out[8+i] = (uint8_t)((side >> (8*i)) & 0xff);
}

uint64_t siphash12_u64(uint64_t side, uint64_t idx, const Key128 &key) {
    uint64_t v0 = 0x736f6d6570736575ULL ^ key.k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ key.k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ key.k0;
    uint64_t v3 = 0x7465646279746573ULL ^ key.k1;

    uint8_t m[16];
    pack_message(side, idx, m);

    uint64_t mi0 = 0; for (int i=0;i<8;i++) mi0 |= (uint64_t)m[i] << (8*i);
    uint64_t mi1 = 0; for (int i=0;i<8;i++) mi1 |= (uint64_t)m[8+i] << (8*i);

    v3 ^= mi0; sip_round(v0,v1,v2,v3); v0 ^= mi0;
    v3 ^= mi1; sip_round(v0,v1,v2,v3); v0 ^= mi1;

    // Finalization with b = len<<56, len=16
    uint64_t b = ((uint64_t)16) << 56;
    v3 ^= b; sip_round(v0,v1,v2,v3); v0 ^= b;

    v2 ^= 0xff;
    sip_round(v0,v1,v2,v3);
    sip_round(v0,v1,v2,v3);

    return v0 ^ v1 ^ v2 ^ v3;
}

static inline uint8_t from_hex_nibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return (uint8_t)(10 + (c - 'A'));
    return 0;
}

Key128 key_from_header_hex(const std::string &hex) {
    // Default test key
    Key128 def{0x0706050403020100ULL, 0x0f0e0d0c0b0a0908ULL};
    if (hex.size() != 32) return def;
    uint8_t bytes[16];
    for (int i=0;i<16;i++) {
        uint8_t hi = from_hex_nibble(hex[2*i]);
        uint8_t lo = from_hex_nibble(hex[2*i+1]);
        bytes[i] = (uint8_t)((hi << 4) | lo);
    }
    // Interpret bytes as little endian into k0,k1
    uint64_t k0=0, k1=0;
    for (int i=0;i<8;i++) k0 |= (uint64_t)bytes[i] << (8*i);
    for (int i=0;i<8;i++) k1 |= (uint64_t)bytes[8+i] << (8*i);
    return Key128{k0,k1};
}

} // namespace sip12
