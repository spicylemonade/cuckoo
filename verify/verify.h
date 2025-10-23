#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "cuckoo/graph.h"

namespace verify42 {

bool verify_cycle_42(const cuckoo::Params& params, const std::vector<cuckoo::edge_t>& edges, std::string* err = nullptr);

} // namespace verify42
