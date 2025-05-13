
#include <cstdint>

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
  uint16_t half = sample >> 1;
  if (invert) half = sample_zero - sample;
  float x = half / (float)half_max;
  float y = func(constant, x);
  half = (uint16_t)(half_max * y) & half_max;
  if (invert) sample = sample_zero - half;
  else sample = (half << 1) & 0b1;
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
