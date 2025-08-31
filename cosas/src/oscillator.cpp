
#include <iostream>

#include "cosas/debug.h"
#include "cosas/constants.h"
#include "cosas/engine_old.h"
#include "cosas/oscillator.h"


BaseOscillator::BaseOscillator(uint32_t f, Wavetable* t) : frequency(f), abs_source(t) {};

int16_t BaseOscillator::next(const int32_t delta, const int32_t phi) {
  /*
   * the RelSource interface deals in delta samples - typically 1, but allowing
   * for more in case the output buffer underflows.  here we need to convert that
   * into absolute time for the AbsSource interfaces, which are all wavetables.
   * since they are all wavetables they all loop at SAMPLE_RATE.  snice frequency
   * is in SUBSAMPLE_BITS units you might naively think that we can calculate
   * time modulo SAMPLE_RATE * (SAMPLE_RATE << SUBSAMPLE_BITS).  actually a
   * factor of 0.5 lower because frequency cannot exceed SAMPLE_RATE / 2.
   *
   * unfortunately that exceeds 32 bits (just!).
   *
   * but since we know that the wavetables all work modulo SAMPLE_BITS we don't
   * need to store more than that.  well, allowing for SUBSAMPLE_BITS which we
   * discard before calling the AbsSource interface.  so we can track time in
   * 32 bits after all.
   */
  // increment time
  uint32_t frequency_val = LOAD(frequency);
  tick += delta * frequency_val;
  if (tick > TIME_MODULUS) tick -= TIME_MODULUS;
  // convert phi to something like phase
  const int32_t phi_phase = (phi * frequency_val) >> PHI_FUDGE_BITS;  // arbitrary scaling
  return abs_source->next(tick + phi_phase);
}


FrequencyParam::FrequencyParam(BaseOscillator* o)
  : Param(0.5, 0, true,
    1.0 / (1 << SUBTICK_BITS), 0.5 * SAMPLE_RATE), oscillator(o) {}

void FrequencyParam::set_oscillator(const uint32_t f) const {  // const because it affects chained oscillator, not us
  oscillator->frequency = f;
}


AbsFreqParam::AbsFreqParam(BaseOscillator* o, const float f) : FrequencyParam(o), frequency(hz2freq(f)) {
  set_oscillator(frequency);
};

void AbsFreqParam::set(const float f) {
  frequency = hz2freq(f);
  set_oscillator(frequency);
  set_relative_freqs(frequency);
}

float AbsFreqParam::get() {
  return freq2hz(frequency);
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

RelFreqParam::RelFreqParam(BaseOscillator* o, AbsFreqParam& ref, float r, float d)
  : FrequencyParam(o), root(ref.get_frequency()), ratio(SimpleRatio(r)), detune(scale2mult_shift8(d)), detune_param(this) {
  recalculate();
}

void RelFreqParam::set(float v) {
  ratio = SimpleRatio(v);
  recalculate();
};

float RelFreqParam::get() {
  return ratio.as_float();
}

void RelFreqParam::set_detune(float v) {
  detune = scale2mult_shift8(v);
  recalculate();
}

float RelFreqParam::get_detune() {
  return unscale2mult_shift8(detune);
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


RelFreqParam::DetuneParam::DetuneParam(RelFreqParam *f)
  : Param(0.1f, 0, false, 0.5, 2), rel_freq_param(f) {}

void RelFreqParam::DetuneParam::set(float v) {
  rel_freq_param->set_detune(v);
}

float RelFreqParam::DetuneParam::get() {
  return rel_freq_param->get_detune();
}


WavedexMixin::WavedexMixin(BaseOscillator* o, Wavelib& wl) : wavedex(o, wl) {};

WavedexMixin::WavedexParam& WavedexMixin::get_dex_param() {
  return wavedex;
}

WavedexMixin::WavedexParam::WavedexParam(BaseOscillator* o, Wavelib& wl)
  : Param(1, 1, false, 0, wl.size()), oscillator(o), wavelib(wl) {};

void WavedexMixin::WavedexParam::set(float val) {
  const size_t n = wavelib.size() - 1;
  widx = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(val)));
  oscillator->abs_source = &wavelib[widx];
}

float WavedexMixin::WavedexParam::get() {
  return widx;
}



AbsDexOsc::AbsDexOsc(float f, Wavelib& wl, size_t widx)
  : BaseOscillator(hz2freq(f), &wl[widx]), WavedexMixin(this, wl), freq_param(AbsFreqParam(this, f)) {}

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


PolyMixin::CtrlParam::CtrlParam(size_t hi, PolyMixin& m, std::function<bool(float)> s, std::function<float()> g)
  : Param(1, 1, false, 0, hi), mixin(m), delegate_set(s), delegate_get(g) {};

void PolyMixin::CtrlParam::set(float v) {
  if (delegate_set(v)) {
    BaseDebug::log("update", v);
    mixin.update();
  }
}

float PolyMixin::CtrlParam::get() {
  return delegate_get();
}

PolyMixin::PolyMixin(BaseOscillator* o, size_t s, size_t a, size_t off)
  : oscillator(o), shape(s), asym(a), offset(off) {
  shape_param = std::move(std::make_unique<CtrlParam>(PolyTable::N_SHAPES, *this,
    [this](float v) noexcept -> bool {size_t s = shape; shape = static_cast<size_t>(v); return s != shape;},
    [this]() noexcept -> float {return shape;}));
  asym_param = std::move(std::make_unique<CtrlParam>(PolyTable::N_SHAPES, *this,
    [this](float v) noexcept -> bool {size_t a = asym; asym = static_cast<size_t>(v); return  a != asym;},
    [this]() noexcept -> float {return asym;}));
  offset_param = std::move(std::make_unique<CtrlParam>(HALF_TABLE_SIZE, *this,
    [this](float v) noexcept -> bool {size_t o = offset; offset = static_cast<size_t>(v); return o != offset;},
    [this]() noexcept -> float {return offset;}));
  update();
}

Param& PolyMixin::get_shp_param() const {
  return *shape_param;
}

Param& PolyMixin::get_asym_param() const {
  return *asym_param;
}

Param& PolyMixin::get_off_param() const {
  return *offset_param;
}

void PolyMixin::update() {
  // BaseDebug::log("start update for", shape, asym, offset);
  std::unique_ptr<Wavetable> save = std::move(wtable);  // save while we modify
  wtable = std::move(std::make_unique<PolyTable>(shape, asym, offset));
  oscillator->abs_source = wtable.get();  // now old value can disappear
  // BaseDebug::log("update done");
}


AbsPolyOsc::AbsPolyOsc(float f, size_t shp, size_t a, size_t off)
  : BaseOscillator(hz2freq(f), nullptr), PolyMixin(this, shp, a, off), freq_param(AbsFreqParam(this, f)) {}

AbsFreqParam& AbsPolyOsc::get_freq_param() {
  return freq_param;
}


RelPolyOsc::RelPolyOsc(size_t shp, size_t asym, size_t off, AbsFreqParam& root, float f, float d)
  : BaseOscillator(0, nullptr), PolyMixin(this, shp, asym, off), freq_param(RelFreqParam(this, root, f, d)) {
  get_freq_param().set(f);  // push initial value
}

RelFreqParam& RelPolyOsc::get_freq_param() {
  return freq_param;
}

