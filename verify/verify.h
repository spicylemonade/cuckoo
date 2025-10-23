#pragma once
#include <vector>
#include <string>
#include <utility>
#include "cuckoo/graph.h"

namespace cuckoo_sip {

struct VerifyResult {
  bool ok{false};
  std::string error;
};

VerifyResult verify_42_cycle(const Graph& G, const std::vector<uint32_t>& edges);

} // namespace cuckoo_sip
