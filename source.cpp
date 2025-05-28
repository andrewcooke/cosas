
#include <cmath>

#include "source.h"


int16_t Source::next(int32_t tick) const {
  return next(tick, 0);
}


size_t tick2idx(int32_t tick) {
  while (tick < 0) tick += static_cast<int32_t>(full_table_size);
  return static_cast<size_t>(tick) >> subtick_bits;
}

int32_t hz2tick(float hz) {
  return static_cast<int32_t>(hz * (1 << subtick_bits));
}

uint32_t hz2freq(float hz) {
  return static_cast<uint32_t>(abs(hz) * (1 << subtick_bits));
}
