
#include <cmath>
#include <iostream>

#include "cosas/source.h"


int16_t Source::next(int32_t tick) const {
  return next(tick, 0);
}


size_t tick2idx(int32_t tick, int32_t phi) {
  // TODO - % may be expensive
  tick = (tick % static_cast<int32_t>(FULL_TABLE_SUB)) + phi;
  return (static_cast<uint32_t>(tick) >> SUBTICK_BITS) % FULL_TABLE_SIZE;
}

int32_t hz2tick(const float hz) {
  return static_cast<int32_t>(hz * (1 << SUBTICK_BITS));
}

uint32_t hz2freq(const float hz) {
  return static_cast<uint32_t>(fabsf(hz) * (1 << SUBTICK_BITS));
}
