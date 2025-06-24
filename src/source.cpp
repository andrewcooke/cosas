
#include <cmath>
#include <iostream>

#include "cosas/source.h"


int16_t Source::next(int32_t tick) const {
  return next(tick, 0);
}


size_t tick2idx(int32_t tick, int32_t phi) {
  // TODO - % may be expensive
  tick = (tick % static_cast<int32_t>(full_table_sub)) + phi;
  return (static_cast<uint32_t>(tick) >> subtick_bits) % full_table_size;
}

int32_t hz2tick(float hz) {
  return static_cast<int32_t>(hz * (1 << subtick_bits));
}

uint32_t hz2freq(float hz) {
  return static_cast<uint32_t>(abs(hz) * (1 << subtick_bits));
}
