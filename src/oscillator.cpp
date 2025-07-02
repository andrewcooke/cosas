
#include <iostream>

#include "cosas/constants.h"
#include "cosas/oscillator.h"
#include "cosas/engine.h"


BaseOscillator::BaseOscillator(uint32_t f, Wavetable* t) : frequency(f), wavetable(t) {};

int16_t BaseOscillator::next(const int32_t tick, const int32_t phi) const {
  // convert phi from sample_min-sample_max to -pi-pi (kinda)
  const int64_t phi_tmp = phi * frequency;
  // fudge allows more variation (phi limited to sample_max)
  // but may need to worry about gain sensitivity
  const auto phi_frac = static_cast<int32_t>(phi_tmp >> (SAMPLE_BITS - 1 - PHI_FUDGE_BITS));
  return wavetable->next(static_cast<int32_t>(tick * frequency), phi_frac);
}


FrequencyParam::FrequencyParam(BaseOscillator* o) : oscillator(o) {}

void FrequencyParam::set_oscillator(const uint32_t f) const {  // const because it affects chained oscillator, not us
  oscillator->frequency = f;
}


// TODO - what's hapoening here w frequency and set_oscillator and types?
AbsFreqParam::AbsFreqParam(BaseOscillator* o, const float f) : FrequencyParam(o), frequency(f) {
  set_oscillator(frequency);
};

void AbsFreqParam::set(const float f) {
  set_oscillator(f);
  set_relative_freqs(frequency);
}

uint32_t AbsFreqParam::get_frequency() const {
  return frequency;
}

void AbsFreqParam::add_relative_freq(RelFreqParam* f) {
  relative_freqs.push_back(f);
}

void AbsFreqParam::set_relative_freqs(const uint32_t f) const {
  for (RelFreqParam* r: relative_freqs) r->set_root(f);
}

RelFreqParam::RelFreqParam(RelDexOsc* o, AbsFreqParam& ref, float r, float d)
  : FrequencyParam(o), root(ref.get_frequency()), ratio(SimpleRatio(r)), detune(scale2mult_shift8(d)), detune_param(this) {
  recalculate();
}

void RelFreqParam::set(float v) {
  ratio = SimpleRatio(v);
  recalculate();
};

void RelFreqParam::set_detune(float v) {
  detune = scale2mult_shift8(v);
  recalculate();
}

void RelFreqParam::set_root(uint32_t f) {
  root = f;
  recalculate();
}

void RelFreqParam::recalculate() {
  uint32_t f = static_cast<uint32_t>(mult_shift8(detune, static_cast<int32_t>(ratio.multiply(root))));
  set_oscillator(f);
}

RelFreqParam::DetuneParam& RelFreqParam::get_det_param() {
  return detune_param;
}


RelFreqParam::DetuneParam::DetuneParam(RelFreqParam *f) : rel_freq_param(f) {}

void RelFreqParam::DetuneParam::set(float v) {
  rel_freq_param->set_detune(v);
}


WavedexMixin::WavedexMixin(BaseOscillator* o, Wavelib& wl) : wavedex(o, wl) {};

WavedexMixin::WavedexParam& WavedexMixin::get_dex_param() {
  return wavedex;
}

WavedexMixin::WavedexParam::WavedexParam(BaseOscillator* o, Wavelib& wl) : oscillator(o), wavelib(wl) {};

void WavedexMixin::WavedexParam::set(float val) {
  size_t n = wavelib.size() - 1;
  size_t widx = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(val)));
  oscillator->wavetable = &wavelib[widx];
}


AbsDexOsc::AbsDexOsc(float f, Wavelib& wl, size_t widx)
  : BaseOscillator(0, &wl[widx]), WavedexMixin(this, wl), freq_param(AbsFreqParam(this, f)) {
  get_freq_param().set(f);  // push initial value
}

AbsFreqParam& AbsDexOsc::get_freq_param() {
  return freq_param;
}


RelDexOsc::RelDexOsc(Wavelib& wl, size_t widx, AbsFreqParam& root, float f, float d)
  : BaseOscillator(0, &wl[widx]), WavedexMixin(this, wl), freq_param(RelFreqParam(this, root, f, d)) {
  get_freq_param().set(f);  // push initial value
}


RelFreqParam& RelDexOsc::get_freq_param() {
  return freq_param;
}


PolyMixin::CtrlParam::CtrlParam(PolyMixin& m, std::function<void(float)> d) : mixin(m), delegate(d) {};

void PolyMixin::CtrlParam::set(float v) {
  delegate(v);
  mixin.update();
}

// no except here to get rid of a complex compiler error i did not understand
PolyMixin::PolyMixin(BaseOscillator* o, size_t s, size_t a, size_t off)
  : oscillator(o), shape(s), asym(a), offset(off) {
  shape_param = std::move(std::make_unique<CtrlParam>(*this, [this](float v) noexcept {shape = v;}));
  asym_param = std::move(std::make_unique<CtrlParam>(*this, [this](float v) noexcept {asym = v;}));
  offset_param = std::move(std::make_unique<CtrlParam>(*this, [this](float v) noexcept {offset = v;}));
  update();
}

Param& PolyMixin::get_shp_param() {
  return *shape_param;
}

Param& PolyMixin::get_asym_param() {
  return *asym_param;
}

Param& PolyMixin::get_off_param() {
  return *offset_param;
}

void PolyMixin::update() {
  std::unique_ptr<Wavetable> save = std::move(wtable);  // save while we modify
  wtable = std::move(std::make_unique<PolyTable>(shape, asym, offset));
  oscillator->wavetable = wtable.get();  // now old value can disappear
}


AbsPolyOsc::AbsPolyOsc(float f, size_t shp, size_t a, size_t off)
  : BaseOscillator(0, nullptr), PolyMixin(this, shp, a, off), freq_param(AbsFreqParam(this, f)) {
  get_freq_param().set(f);  // push initial value
}

AbsFreqParam& AbsPolyOsc::get_freq_param() {
  return freq_param;
}

