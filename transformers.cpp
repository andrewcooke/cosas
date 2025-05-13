
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
/*
Compander::Compander(Node& nd, float g)
  : Transformer(nd), gamma(g) {};

uint16_t Compander::next(int64_t tick, int32_t phi) {
  uint16_t sample = node.next(tick, phi);
  bool invert = !(sample & sample_zero);
  uint16_t half = sample >> 1;
  if (invert) half = sample_zero - sample;
  half = (half_max * pow(half / (float)half_max, gamma)) & half_max;
  if (invert) sample = sample_zero - half;
  else sample = (half <<) 1 & 0b1;
  return sample;
}
*/

Compander::Compander(Node& nd, float g)
  : Transformer(nd), gamma(g) {};

uint16_t Compander::next(int64_t tick, int32_t phi) {
  uint16_t sample = node.next(tick, phi);
  bool invert = !(sample & sample_zero);
  uint16_t half = sample >> 1;
  if (invert) half = sample_zero - sample;
  half = (uint16_t)(half_max * pow(half / (float)half_max, gamma)) & half_max;
  if (invert) sample = sample_zero - half;
  else sample = (half << 1) & 0b1;
  return sample;
}

