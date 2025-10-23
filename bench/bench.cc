#include "bench/bench.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace benchrun {

using namespace cuckoo;

static bool parse_hex_16(const std::string& hex, siphash::Key& key_out) {
  std::string s;
  for (char c : hex) {
    if (("0" <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) s.push_back(c);
  }
  if (s.size() < 32) s.append(32 - s.size(), '0');
  if (s.size() > 32) s = s.substr(0, 32);
  auto hexbyte = [](char a, char b) -> uint8_t {
    auto h = [](char x) -> uint8_t {
      if ('0' <= x && x <= '9') return uint8_t(x - '0');
      if ('a' <= x && x <= 'f') return uint8_t(x - 'a' + 10);
      if ('A' <= x && x <= 'F') return uint8_t(x - 'A' + 10);
      return 0;
    };
    return (uint8_t)((h(a) << 4) | h(b));
  };
  uint8_t bytes[16];
  for (int i = 0; i < 16; ++i) bytes[i] = hexbyte(s[2*i], s[2*i+1]);
  auto le64 = [](const uint8_t* p) -> uint64_t {
    return (uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) |
           ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
  };
  key_out.k0 = le64(bytes);
  key_out.k1 = le64(bytes + 8);
  return true;
}

BenchResult run(const BenchConfig& cfg) {
  BenchResult R;
  R.attempts = cfg.attempts;

  HashVariant hv = (cfg.hash == "sip24") ? HashVariant::Sip24 : HashVariant::Sip12;

  siphash::Key base_key{0x1234567890abcdefULL, 0xfedcba0987654321ULL};
  if (!cfg.header_hex.empty()) {
    parse_hex_16(cfg.header_hex, base_key);
  }

  LeanSolverConfig lcfg;
  lcfg.trim_rounds = 32;
  lcfg.memcap_bytes_per_edge = cfg.memcap_bytes_per_edge;
  LeanSolver lean(lcfg);

  MeanSolver mean;

  R.bytes_per_edge = (cfg.mode == "lean") ? lean.theoretical_bytes_per_edge(cfg.edge_bits) : 0.0;

  std::mt19937_64 rng(0xC0FFEEULL);

  auto t_start = std::chrono::high_resolution_clock::now();
  for (uint32_t att = 0; att < cfg.attempts; ++att) {
    uint64_t salt0 = rng();
    uint64_t salt1 = rng();
    siphash::Key key{base_key.k0 ^ salt0 ^ (uint64_t)att, base_key.k1 ^ salt1 ^ ((uint64_t)att << 1)};

    Params params = make_params(cfg.edge_bits, key, hv);

    auto one_start = std::chrono::high_resolution_clock::now();
    bool ok = false;
    if (cfg.mode == "lean") {
      Solution sol;
      std::string msg;
      ok = lean.solve(params, sol, &msg);
      if (ok) {
        std::string verr;
        if (!verify42::verify_cycle_42(params, sol.edges, &verr)) {
          ok = false;
        }
      }
    } else {
      MeanSolution sol;
      std::string msg;
      ok = mean.solve(params, sol, &msg);
      if (ok) {
        std::string verr;
        if (!verify42::verify_cycle_42(params, sol.edges, &verr)) ok = false;
      }
    }
    auto one_end = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration<double>(one_end - one_start).count();

    if (ok) {
      R.successes += 1;
      R.success_times_sec.push_back(secs);
    }
  }
  auto t_end = std::chrono::high_resolution_clock::now();
  R.total_wall_sec = std::chrono::duration<double>(t_end - t_start).count();
  return R;
}

} // namespace benchrun
