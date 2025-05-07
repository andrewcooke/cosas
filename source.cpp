
import std;
using namespace std;

#include "constants.h"
#include "source.h"


FreqScale::FreqScale(uint16_t n, uint16_t d) : numerator(n), denominator(d) {
  three = (d % 3) == 0;
  if (three) d /= 3;
  bits = 0;
  while (d > 1) {
    if (d & 1) throw invalid_argument("denominator should be 2^n x [3,1]");
    bits++; d >>= 1;
  }
};

uint16_t FreqScale::scale(uint16_t freq) const {
  uint32_t t = freq * numerator;
  if (three) t /= 3;  // https://stackoverflow.com/a/171369
  return uclip(t >> bits);
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


// amplitude scaling factor is converted to 16 bits where the top 8 bits are integer and the bottom 8 fractional
// after multiplying the result is right shifted 8 bits to drop the fractional part
// this is done in 32 bits to avoid clipping intermediate values

const int one_bits = 8;
const uint32_t one = 1 << one_bits;

AmpScale::AmpScale(float factor) : factor(factor), norm(factor * one) {};

uint16_t AmpScale::scale(uint16_t amp) const {
  return uclip(((uint32_t)amp * norm) >> one_bits);
};


