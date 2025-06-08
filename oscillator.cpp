
#include <iostream>

#include "constants.h"
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavelib& wl, size_t widx): wavedex(this, wl), wavetable(wl[widx]) {};

Oscillator::Wavedex& Oscillator::get_wavedex() {
  return wavedex;
}

int16_t Oscillator::next(int32_t tick, int32_t phi) const {
  int64_t freq = frequency;
  // convert phi from sample_min-sample_max to -pi-pi (kinda)
  int64_t phi_tmp = phi * freq;
  // fudge allows more variation (phi limited to sample_max)
  // but may need to worry about gain sensitivity
  int32_t phi_frac = phi_tmp >> (sample_bits - 1 - phi_fudge_bits);
  return wavetable.next(tick * freq, phi_frac);
}

Oscillator::Wavedex::Wavedex(Oscillator* o, Wavelib& wl) : oscillator(o), wavelib(wl) {};

void Oscillator::Wavedex::set(float val) {
  size_t n = wavelib.size() - 1;
  size_t widx = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(val)));
  std::cerr << "widx " << widx << std::endl;
  oscillator->wavetable = wavelib[widx];
}



Frequency::Frequency(Oscillator* o) : oscillator(o) {}

void Frequency::set_oscillator(uint32_t f) {
  std::cerr << "set osc " << f << std::endl;
  oscillator->frequency = f;
}


AbsoluteFreq::AbsoluteFreq(AbsoluteOsc* o, float f)
  : Frequency(o), frequency(hz2freq(f)), relative_freqs() {
  std::cerr << "AF created " << relative_freqs.size() << std::endl;
  set_oscillator(frequency);  
};

void AbsoluteFreq::set(float f) {
  frequency = hz2freq(f);
  set_oscillator(frequency);
  set_relative_freqs(frequency);
}

uint32_t AbsoluteFreq::get_frequency() const {
  return frequency;
}

void AbsoluteFreq::add_relative_freq(RelativeFreq* f) {
  relative_freqs.push_back(f);
}

void AbsoluteFreq::set_relative_freqs(uint32_t f) {
  for (RelativeFreq* r: relative_freqs) r->set_root(f);
}


RelativeFreq::RelativeFreq(RelativeOsc* o, float f, SimpleRatio r, float d)
  : RelativeFreq(o, hz2freq(f), r, d) {}

// prolly need to move this to inside constructor?
RelativeFreq::RelativeFreq(RelativeOsc* o, uint32_t f, SimpleRatio r, float d)
  : Frequency(o), root(f), ratio(r), detune(scale2mult_shift8(d)), detune_param(this) {
  recalculate();
}

RelativeFreq::RelativeFreq(RelativeOsc* o, AbsoluteFreq& ref, SimpleRatio r, float d)
  : RelativeFreq(o, ref.get_frequency(), r, d) {}

RelativeFreq::RelativeFreq(RelativeOsc* o, AbsoluteFreq& ref, SimpleRatio r)
  : RelativeFreq(o, ref, r, 1) {}

RelativeFreq::RelativeFreq(RelativeOsc* o, AbsoluteFreq& ref, float r, float d)
  : RelativeFreq(o, ref, SimpleRatio(r), d) {}

RelativeFreq::RelativeFreq(RelativeOsc* o, AbsoluteFreq& ref, float r)
  : RelativeFreq(o, ref, r, 1) {}

void RelativeFreq::set(float v) {
  ratio = SimpleRatio(v);
  recalculate();
};

void RelativeFreq::set_detune(float v) {
  detune = scale2mult_shift8(v);
  recalculate();
}

void RelativeFreq::set_root(uint32_t f) {
  root = f;
  recalculate();
}

void RelativeFreq::recalculate() {
  uint32_t f = static_cast<uint32_t>(mult_shift8(detune, static_cast<int32_t>(ratio.multiply(root))));
  set_oscillator(f);
}

RelativeFreq::Detune& RelativeFreq::get_detune() {
  return detune_param;
}


RelativeFreq::Detune::Detune(RelativeFreq *f) : frequency(f) {}

void RelativeFreq::Detune::set(float v) {
  frequency->set_detune(v);
}


AbsoluteOsc::AbsoluteOsc(Wavelib& wl, size_t widx, float f) : Oscillator(wl, widx), freq_param(AbsoluteFreq(this, f)) {
  get_param().set(f);  // push initial value
}

AbsoluteFreq& AbsoluteOsc::get_param() {
  return freq_param;
}


RelativeOsc::RelativeOsc(Wavelib& wl, size_t widx, AbsoluteFreq& root, float f, float d) : Oscillator(wl, widx), freq_param(RelativeFreq(this, root, f, d)) {}

RelativeFreq& RelativeOsc::get_param() {
  return freq_param;
}

