
#include "cosas/maths.h"
#include "cosas/modulators.h"


FM::FM(RelSource& car, RelSource& mod)
  : carrier(car), modulator(mod) {};

int16_t FM::next(const int32_t phi) {
  const int32_t phi2 = modulator.next(phi);
  return carrier.next(phi2);
};


AM::AM(RelSource& src1, RelSource& src2)
  : src1(src1), src2(src2) {};

int16_t AM::next(int32_t phi) {
  const int32_t s1 = src1.next(phi);
  const int32_t s2 = src2.next(phi);
  return clip_16((s1 * s2) >> 16);
};

