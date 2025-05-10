
import std;
using namespace std;

#include "constants.h"
#include "maths.h"
#include "source.h"


AbsoluteFreq::AbsoluteFreq(uint16_t freq) : frequency(freq) {};

uint16_t AbsoluteFreq::get() const {
  return frequency;
}

void AbsoluteFreq::set(uint16_t freq) {
  frequency = freq;
}


RelativeFreq::RelativeFreq(const Frequency& ref, uint16_t num, uint16_t denom) :
  reference(ref), numerator(num) {
  if (num != 0 && denom != 0) {
    // remove common factors
    uint16_t k = gcd(numerator, denom);
    if (k > 1) {numerator /= k; denom /= k;}
    denom_three = (denom % 3) == 0; if (denom_three) denom /= 3;
    denom_five = (denom % 5) == 0; if (denom_five) denom /= 5;
    denom_bits = 0;
    while (denom > 1) {
      if (denom & 1) throw invalid_argument("denominator should be 2^n x [3,1] x [5,1]");
      denom_bits++; denom >>= 1;
    }
  }
};

uint16_t RelativeFreq::get() const {
  uint16_t freq = reference.get();
  if (denom_bits != 0 || denom_three || denom_five) {
    freq *= numerator;
    if (denom_three) freq /= 3;  // https://stackoverflow.com/a/171369
    if (denom_five) freq /= 5;
    freq = clip_u16(freq >> denom_bits);
  }
  return freq;
}


// amplitude scaling factor is converted to 16 bits where the top 8 bits are integer and the bottom 8 fractional
// after multiplying the result is right shifted 8 bits to drop the fractional part
// this is done in 32 bits to avoid clipping intermediate values

const int one_bits = 8;
const uint32_t one = 1 << one_bits;

Amplitude::Amplitude(float factor) : factor(factor), norm(factor * one) {};

uint16_t Amplitude::scale(uint16_t amp) const {
  return clip_u16(((uint32_t)amp * norm) >> one_bits);
};


Balance::Balance(float wet) : wet(wet), wet_weight(wet * one), dry_weight((1 - wet) * one) {};

uint16_t Balance::combine(uint16_t dry, uint16_t wet) const {
  return clip_u16((wet_weight * wet + dry_weight * dry) >> one_bits);
}
