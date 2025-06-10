
#include <iostream>

#include "constants.h"
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavelib& wl, size_t widx): wavedex(this, wl), wavetable(&wl[widx]) {};

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
  std::cerr << "using " << &wavetable << std::endl;
  return wavetable->next(tick * freq, phi_frac);
}

Oscillator::Wavedex::Wavedex(Oscillator* o, Wavelib& wl) : oscillator(o), wavelib(wl) {};

void Oscillator::Wavedex::set(float val) {
  size_t n = wavelib.size() - 1;
  size_t widx = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(val)));
  oscillator->wavetable = &wavelib[widx];
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

RelativeFreq::RelativeFreq(RelativeOsc* o, AbsoluteFreq& ref, float r, float d)
  : Frequency(o), root(ref.get_frequency()), ratio(SimpleRatio(r)), detune(scale2mult_shift8(d)), detune_param(this) {
  recalculate();
}

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


TEST_CASE("Wavedex") {
  Wavelib w = Wavelib();
  AbsoluteOsc o = AbsoluteOsc(w, w.sine_gamma_1, 4400);
  CHECK(o.next(1000, 0) == -32417);
  o.get_wavedex().set(w.square_duty_05);
  CHECK(o.next(1000, 0) == -32767);
}

TEST_CASE("Detune") {
  Wavelib w = Wavelib();
  AbsoluteOsc o1 = AbsoluteOsc(w, w.sine_gamma_1, 4400);
  RelativeOsc o2 = RelativeOsc(w, w.sine_gamma_1, o1.get_param(), 1, 1);
  CHECK(o2.next(1000, 0) == -32417);
  o2.get_param().get_detune().set(0.9);
  CHECK(o2.next(1000, 0) != -32417);
}
