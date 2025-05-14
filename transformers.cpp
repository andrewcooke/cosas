
#include <cstdint>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "transformers.h"


Gain::Gain(Node& nd, const Amplitude& amp)
  : Transformer(nd), amplitude(amp) {};

uint16_t Gain::next(int64_t tick, int32_t phi) {
  return amplitude.scale(node.next(tick, phi));
}


// these (float based) may be too slow?

OneParFunc::OneParFunc(Node& nd, float k)
  : Transformer(nd), constant(k) {};

uint16_t OneParFunc::next(int64_t tick, int32_t phi) {
  uint16_t sample = node.next(tick, phi);
  bool invert = !(sample & sample_zero);
  uint16_t half = sample - sample_zero;
  if (invert) half = sample_zero - sample;
  float x = half / (float)half_max;
  float y = func(constant, x);
  half = (uint16_t)(half_max * y) & half_max;
  if (invert) sample = sample_zero - half;
  else sample = half + sample_zero;
  return sample;
}


Compander::Compander(Node& nd, float k)
  : OneParFunc(nd, k) {};

float Compander::func(float k, float x) const {
  return pow(x, k);
}


Folder::Folder(Node& nd, float k)
  : OneParFunc(nd, k) {};

// first half goes from flat to curve
// second half actually folds
float Folder::func(float k, float x) const {
  if (k < 1) return x * (1 + k * (1 - x));
  else return 1 - pow(k * x - 1, 2);
}


TEST_CASE("Folder") {

  Constant c12 = Constant(1 << 12);  // random -ve value
  Folder f0_12 = Folder(c12, 0);
  CHECK(f0_12.next(0, 0) == 1 << 12);
  Folder f1_12 = Folder(c12, 1);
  CHECK(f1_12.next(0, 0) == 513);  // not sure if correct, but more -ve
  Constant c12x = Constant(sample_zero + (sample_zero - (uint16_t)(1 << 12)));
  Folder f1_12x = Folder(c12x, 1);
  CHECK(f1_12x.next(0, 0) == sample_max - 513 + 1);  // symmetrical (within a fudge)
  
  Constant cmax = Constant(sample_max);  
  Folder f0_max = Folder(cmax, 0);
  CHECK(f0_max.next(0, 0) == sample_max);
  Folder f1_max = Folder(cmax, 1);
  CHECK(f1_max.next(0, 0) == sample_max);
  Folder f2_max = Folder(cmax, 2);
  CHECK(f2_max.next(0, 0) == sample_zero);

}
