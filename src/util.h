#ifndef CUCKOO_SIP_UTIL_H
#define CUCKOO_SIP_UTIL_H

#include <cstdint>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <optional>

#include "siphash12.h"

namespace cuckoo_sip {

uint64_t fnv1a64(const void* data, size_t len);

// Deterministic 128-bit key from a header string
SipHashKey derive_key_from_header(const std::string& header);

// Optional: parse a 16-byte hex string (32 hex chars) into key
std::optional<SipHashKey> parse_hex_key128(const std::string& hex);

// Produce random header bytes as hex string (32 hex chars)
std::string random_hex_header();

// Timing helper
inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace cuckoo_sip

#endif // CUCKOO_SIP_UTIL_H
