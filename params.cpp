
#include "constants.h"
#include "maths.h"
#include "params.h"
#include "source.h"


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
