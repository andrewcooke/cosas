
#include "constants.h"
#include "maths.h"
#include "params.h"
#include "source.h"


// some scaling factors are converted to 16 bits where the top 8 bits
// are integer and the bottom 8 fractional.  after multiplying the
// result is right shifted 8 bits to drop the fractional part.  this
// is done in 32 bits to avoid clipping intermediate values

const int one_bits = 8;
const uint32_t one = 1 << one_bits;

inline int32_t scale2mult_shift(float f) {
  return f * one;
}

inline int16_t mult_shift(int32_t k, int16_t x) {
  return clip_16((k * x) >> one_bits);
}

inline int32_t mult_shift(int32_t k, int32_t x) {
  return (k * (int64_t)x) >> one_bits;
}


AbsoluteFreq::AbsoluteFreq(float freq) : frequency(hz2tick(freq)) {};

AbsoluteFreq& AbsoluteFreq::get_root() {
  return *this;
}

uint32_t AbsoluteFreq::get_frequency() const {
  return frequency;
}


RelativeFreq::RelativeFreq(Frequency& ref, SimpleRatio r, float d)
  : reference(ref), ratio(r), detune(scale2mult_shift(d)) {};

RelativeFreq::RelativeFreq(Frequency& ref, SimpleRatio r)
  : RelativeFreq(ref, r, 1) {};

RelativeFreq::RelativeFreq(Frequency& ref, float r, float d)
  : reference(ref), ratio(SimpleRatio(r)), detune(scale2mult_shift(d)) {};

RelativeFreq::RelativeFreq(Frequency& ref, float r)
  : RelativeFreq(ref, r, 1) {};

AbsoluteFreq& RelativeFreq::get_root() {
  return reference.get_root();
}

uint32_t RelativeFreq::get_frequency() const {
  return  mult_shift(detune, ratio.multiply(reference.get_frequency()));
}


Amplitude::Amplitude() : Amplitude(1) {};

Amplitude::Amplitude(float f) : factor(scale2mult_shift(f)) {};

int16_t Amplitude::scale(int16_t amp) const {
  return mult_shift(factor, amp);
};


Balance::Balance() : Balance(1) {};

Balance::Balance(float wet)
  : wet_weight(scale2mult_shift(wet)), dry_weight(scale2mult_shift(1 - wet)) {};

int16_t Balance::combine(int16_t dry, int16_t wet) const {
  return clip_16((wet_weight * wet + dry_weight * dry) >> one_bits);
}
