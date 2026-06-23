#pragma once

#include <random>

#include "types.h"

namespace arch::core {

struct UUID {
public:
  UUID() {
    static std::random_device rd;
    static std::mt19937_64 engine(rd());
    static std::uniform_int_distribution<u64> dist;

    _low = dist(engine);
    _high = dist(engine);

    // RFC 4122 Version 4 (Random) bits set
    // Set the 4 bits for version (0100)
    _high = (_high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set the 2 bits for variant (10)
    _low = (_low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
  }

  UUID(u64 low, u64 high) : _low(low), _high(high) {}

  bool operator==(const UUID &other) const {
    return _low == other._low && _high == other._high;
  }
  bool operator!=(const UUID &other) const { return !(*this == other); }

  u64 GetLow() const { return _low; }
  u64 GetHigh() const { return _high; }

private:
  u64 _low;
  u64 _high;
};

} // namespace arch::core