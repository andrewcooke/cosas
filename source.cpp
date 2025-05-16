
#include "source.h"


int16_t Source::next(int64_t tick) {
  return next(tick, 0);
}


int64_t tick2idx(int64_t tick) {
  return tick >> subtick_bits;
}

int64_t hz2tick(float hz) {
  return (int64_t)(hz * (1 << subtick_bits));
}
