
#include "constants.h"
#include "maths.h"
#include "params.h"


AbsoluteFreq::AbsoluteFreq(uint16_t freq) : frequency(freq) {};

AbsoluteFreq& AbsoluteFreq::get_root() {
  return *this;
}

uint16_t AbsoluteFreq::get() const {
  return frequency;
}


RelativeFreq::RelativeFreq(Frequency& ref, SimpleRatio r) :
  reference(ref), ratio(r) {
};

RelativeFreq::RelativeFreq(Frequency& ref, float r) :
  reference(ref), ratio(SimpleRatio(r)) {
};

AbsoluteFreq& RelativeFreq::get_root() {
  return reference.get_root();
}

uint16_t RelativeFreq::get() const {
  return ratio.multiply(reference.get());
}


// amplitude scaling factor is converted to 16 bits where the top 8
// bits are integer and the bottom 8 fractional.  after multiplying
// the result is right shifted 8 bits to drop the fractional part.
// this is done in 32 bits to avoid clipping intermediate values

const int one_bits = 8;
const uint32_t one = 1 << one_bits;


Amplitude::Amplitude() : Amplitude(1) {};

Amplitude::Amplitude(float factor) : factor(factor), norm(factor * one) {};

int16_t Amplitude::scale(int16_t amp) const {
  return clip_16((amp * norm) >> one_bits);
};


Balance::Balance() : Balance(1) {};

Balance::Balance(float wet) : wet(wet), wet_weight(wet * one), dry_weight((1 - wet) * one) {};

int16_t Balance::combine(int16_t dry, int16_t wet) const {
  return clip_16((wet_weight * wet + dry_weight * dry) >> one_bits);
}
