
#include "cosas/maths.h"
#include "cosas/modulators.h"


FM::FM(PhaseSource& car, PhaseSource& mod)
  : carrier(car), modulator(mod) {};

int16_t FM::next(uint32_t tick, int32_t phi) {
  const int32_t phi2 = modulator.next(tick, phi);
  return carrier.next(tick, phi2);
};


AM::AM(PhaseSource& src1, PhaseSource& src2)
  : src1(src1), src2(src2) {};

int16_t AM::next(uint32_t tick, int32_t phi) {
  const int32_t s1 = src1.next(tick, phi);
  const int32_t s2 = src2.next(tick, phi);
  return clip_16((s1 * s2) >> 16);
};

