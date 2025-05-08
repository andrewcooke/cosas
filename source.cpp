
import std;
using namespace std;

#include "constants.h"
#include "source.h"


Multiplier::Multiplier(uint16_t n, uint16_t d) : numerator(n), denominator(d) {
  three = (d % 3) == 0; if (three) d /= 3;
  five = (d % 5) == 0; if (five) d /= 5;
  bits = 0;
  while (d > 1) {
    if (d & 1) throw invalid_argument("denominator should be 2^n x [3,1] x [5,1]");
    bits++; d >>= 1;
  }
};

uint16_t Multiplier::scale(uint16_t freq) const {
  uint32_t t = freq * numerator;
  if (three) t /= 3;  // https://stackoverflow.com/a/171369
  if (five) t /= 5;
  return clip(t >> bits);
}


const uint32_t limit = (1 << bit_depth) - 1;

uint16_t clip(uint32_t inter) {
  if (inter > limit) inter = limit;
  return (uint16_t)inter;
}

uint16_t clip(int32_t inter) {
  if (inter > limit) inter = limit;
  if (inter < 0) inter = 0;
  return (uint16_t)inter;
}

uint16_t clip(float inter) {
  return clip((int32_t)inter);
}


// amplitude scaling factor is converted to 16 bits where the top 8 bits are integer and the bottom 8 fractional
// after multiplying the result is right shifted 8 bits to drop the fractional part
// this is done in 32 bits to avoid clipping intermediate values

const int one_bits = 8;
const uint32_t one = 1 << one_bits;

Amplitude::Amplitude(float factor) : factor(factor), norm(factor * one) {};

uint16_t Amplitude::scale(uint16_t amp) const {
  return clip(((uint32_t)amp * norm) >> one_bits);
};


Balance::Balance(float wet) : wet(wet), wet_weight(wet * one), dry_weight((1 - wet) * one) {};

uint16_t Balance::combine(uint16_t dry, uint16_t wet) const {
  return clip((wet_weight * wet + dry_weight * dry) >> one_bits);
}
