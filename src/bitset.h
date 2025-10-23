#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace cuckoo_sip {

class DynamicBitset {
 public:
  DynamicBitset() : nbits_(0) {}
  explicit DynamicBitset(uint64_t nbits) { reset_size(nbits); }

  void reset_size(uint64_t nbits) {
    nbits_ = nbits;
    uint64_t nwords = (nbits + 63) / 64;
    bits_.assign(nwords, 0);
  }

  uint64_t size() const { return nbits_; }
  uint64_t words() const { return bits_.size(); }

  void clear_all() { std::fill(bits_.begin(), bits_.end(), 0ULL); }
  void set_all() {
    std::fill(bits_.begin(), bits_.end(), ~0ULL);
    uint64_t excess = words() * 64 - nbits_;
    if (excess) {
      uint64_t mask = (~0ULL) >> excess;
      bits_.back() &= mask;
    }
  }

  inline bool test(uint64_t i) const { return (bits_[i >> 6] >> (i & 63)) & 1ULL; }
  inline void set(uint64_t i) { bits_[i >> 6] |= (1ULL << (i & 63)); }
  inline void reset(uint64_t i) { bits_[i >> 6] &= ~(1ULL << (i & 63)); }
  inline void assign(uint64_t i, bool v) { if (v) set(i); else reset(i); }

  const uint64_t* data() const { return bits_.data(); }
  uint64_t* data() { return bits_.data(); }

  uint64_t count() const {
    uint64_t c = 0;
    for (uint64_t w : bits_) c += __builtin_popcountll(w);
    return c;
  }

 private:
  uint64_t nbits_;
  std::vector<uint64_t> bits_;
};

} // namespace cuckoo_sip
