
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

void AbsoluteFreq::set_frequency(float f) {
  frequency = f;
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

void RelativeFreq::set_ratio(float r) {
  ratio = SimpleRatio(r);
}

void RelativeFreq::set_detune(float d) {
  detune = d;
}


Amplitude::Amplitude(float a) : amplitude(a) {};

Amplitude::Amplitude() : Amplitude(1) {};

int16_t Amplitude::scale(int16_t amp) const {
  return clip_16(amplitude * amp); 
};

void Amplitude::set_amplitude(float a) {
  amplitude = a;
}


Balance::Balance() : Balance(1) {};

Balance::Balance(float w) : wet_weight(w) {};

int16_t Balance::combine(int16_t wet, int16_t dry) const {
  return clip_16(wet_weight * wet + (1 - wet_weight) * dry);
}

void Balance::set_wet_weight(float w) {
  wet_weight = w;
}


Wavedex::Wavedex(Wavelib& wl, size_t idx) : wavelib(wl), wavedex(idx), wavetable(wl[idx]) {};

void Wavedex::set_wavedex(float idx) {
  size_t n = wavelib.size() - 1;
  wavedex = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(idx * n)));
  wavetable = wavelib[wavedex];
}

Wavetable& Wavedex::get_wavetable() const {
  return wavetable;
}

  
