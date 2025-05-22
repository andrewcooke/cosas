
#include "source.h"


int16_t Source::next(int32_t tick) const {
  return next(tick, 0);
}


size_t tick2idx(int32_t tick) {
  return tick >> subtick_bits;
}

int32_t hz2tick(float hz) {
  return (int32_t)(hz * (1 << subtick_bits));
}
