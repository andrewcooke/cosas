
#include <algorithm>
#include <iostream>

#include "constants.h"
#include "maths.h"
#include "params.h"
#include "source.h"


AbsoluteFreq::AbsoluteFreq(float freq) : frequency(hz2freq(freq)) {};

uint32_t AbsoluteFreq::get_frequency() const {
  return frequency;
}


RelativeFreq::RelativeFreq(Frequency& ref, SimpleRatio r, float d)
  : reference(ref), ratio(r), detune(scale2mult_shift8(d)) {};

RelativeFreq::RelativeFreq(Frequency& ref, SimpleRatio r)
  : RelativeFreq(ref, r, 1) {};

RelativeFreq::RelativeFreq(Frequency& ref, float r, float d)
  : reference(ref), ratio(SimpleRatio(r)), detune(scale2mult_shift8(d)) {};

RelativeFreq::RelativeFreq(Frequency& ref, float r)
  : RelativeFreq(ref, r, 1) {};

uint32_t RelativeFreq::get_frequency() const {
  // mult_shift deals w signed but freq is unsigned
  return  static_cast<uint32_t>(mult_shift8(detune, static_cast<int32_t>(ratio.multiply(reference.get_frequency()))));
}

void RelativeFreq::set_detune(float v) {
  detune = scale2mult_shift8(v);
}

RelativeFreq::Detune::Detune(RelativeFreq *f) : freq(f) {};

std::unique_ptr<RelativeFreq::Detune> RelativeFreq::get_detune() {
  return std::make_unique<Detune>(this);
}

void RelativeFreq::Detune::set(float v) {
  freq->set_detune(v);
}


Amplitude::Amplitude(float a) : amplitude(a) {};

Amplitude::Amplitude() : Amplitude(1) {};

int16_t Amplitude::scale(int16_t amp) const {
  float a = amplitude * amp;
  int16_t b = clip_16(a);
  std::cerr << amp << " amp " << this << " (" << amplitude << ") " << a << " " << b << std::endl;
  return b;
};

void Amplitude::set(float f) {
  std::cerr << amplitude << " amp " << this << " set " << f << std::endl;
  amplitude = f;
}

TEST_CASE("Amplitude") {
  Amplitude a = Amplitude();
  CHECK(a.scale(110) == 110);
  a.set(0.1);
  CHECK(a.scale(110) == 11);
}


Balance::Balance() : Balance(1) {};

Balance::Balance(float w) : wet_weight(w) {};

int16_t Balance::combine(int16_t wet, int16_t dry) const {
  return clip_16(wet_weight * wet + (1 - wet_weight) * dry);
}


Wavedex::Wavedex(Wavelib& wl, size_t idx) : wavelib(wl), wavedex(idx), wavetable(wl[idx]) {};

/*
void Wavedex::set_wavedex(float idx) {
  size_t n = wavelib.size() - 1;
  wavedex = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(idx * n)));
  wavetable = wavelib[wavedex];
}
*/

Wavetable& Wavedex::get_wavetable() const {
  return wavetable;
}

  
