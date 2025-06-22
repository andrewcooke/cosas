
#include <iostream>
#include <cstdint>

#include "maths.h"
#include "modulators.h"


FM::FM(const Node& car, const Node& mod)
  : carrier(car), modulator(mod) {};

int16_t FM::next(int32_t tick, int32_t phi) const {
  int32_t delta = modulator.next(tick, phi);
  return carrier.next(tick, delta);
};


AM::AM(const Node& nd1, const Node& nd2)
  : node1(nd1), node2(nd2) {};

int16_t AM::next(int32_t tick, int32_t phi) const {
  int32_t s1 = node1.next(tick, phi);
  int32_t s2 = node2.next(tick, phi);
  return clip_16((s1 * s2) >> 16);
};

