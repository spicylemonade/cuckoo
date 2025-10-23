#include "util.h"

#include <cstring>
#include <sstream>
#include <iomanip>

namespace cuckoo_sip {

uint64_t fnv1a64(const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t hash = 1469598103934665603ULL; // FNV offset
    const uint64_t prime = 1099511628211ULL;
    for (size_t i = 0; i < len; ++i) {
        hash ^= p[i];
        hash *= prime;
    }
    return hash;
}

SipHashKey derive_key_from_header(const std::string& header) {
    // Simple deterministic derivation: FNV-1a of header + tag
    const std::string h0 = header + std::string("/k0");
    const std::string h1 = header + std::string("/k1");
    SipHashKey key{ fnv1a64(h0.data(), h0.size()), fnv1a64(h1.data(), h1.size()) };
    return key;
}

static inline int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::optional<SipHashKey> parse_hex_key128(const std::string& hex) {
    if (hex.size() != 32) return std::nullopt; // 16 bytes => 32 hex chars
    uint8_t bytes[16];
    for (size_t i = 0; i < 16; ++i) {
        int hi = hexval(hex[2*i]);
        int lo = hexval(hex[2*i+1]);
        if (hi < 0 || lo < 0) return std::nullopt;
        bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    // Interpret as little-endian 2x uint64
    uint64_t k0 = 0, k1 = 0;
    std::memcpy(&k0, bytes, 8);
    std::memcpy(&k1, bytes + 8, 8);
    return SipHashKey{ k0, k1 };
}

std::string random_hex_header() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t a = dist(gen), b = dist(gen);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << a << std::setw(16) << b;
    return oss.str();
}

} // namespace cuckoo_sip
