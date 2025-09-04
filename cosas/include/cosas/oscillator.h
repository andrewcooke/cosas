
#ifndef COSAS_OSCILLATOR_H
#define COSAS_OSCILLATOR_H

#include <memory>
#include <functional>

#include "cosas/patomic.h"
#include "cosas/maths.h"
#include "cosas/params.h"
#include "cosas/wavelib.h"


// there is a lot of support structure here that ends up providing two
// types of oscillator, each in two different forms (absolute and relative)

// the "poly" oscillator takes three parameters (shape, asymmetry and offset)
// and generates a single waveform table (per oscillator) at run time.
// shape chooses the shape of the "first part" of the waveform; the "second
// part" uses shape+asym; offset changes the relative size of first and
// second parts.  this is very flexible, with some shapes that could be
// useful for envelopes, and uses relatively little memory, but could be slow.

// the "wavedex" oscillator uses pre-calculated tables of more traditional
// waveforms.  it has a single parameter that selects between tables.
// compared to "poly", it is less flexible, and uses more memory (but the
// tables are shared amongst all oscillators), but is faster in use and
// has a more compact interface.

// the original manager/engines used both types of oscillator.

// for the smallest memory footprint we should use poly.  we could reduce the
// number of parameters by slaving relative oscillators to their absolute
// counterparts.  we should not support persistent engines across changes
// (at least, not by keeping the old engine present - perhaps we could
// save parameters).

// looks up the waveform in a wavetable, given the frequency
class BaseOscillator : public RelSource {
public:
  friend class PolyMixin;
  friend class FrequencyParam;
  friend class WavedexMixin;
  BaseOscillator(uint32_t f, Wavetable *t);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) override;
protected:
  ATOMIC(uint32_t) frequency;  // subtick units
  ATOMIC(AbsSource*) abs_source;
private:
  static constexpr int32_t TIME_MODULUS = SAMPLE_RATE << SUBTICK_BITS;  // see discussion in oscillator.cpp
  int32_t tick = 0;
};


class FrequencyParam : public Param {
public:
  explicit FrequencyParam(BaseOscillator* o);
protected:
  // on change, frequency is sent to controlled oscillator
  void set_oscillator(uint32_t f) const;
  BaseOscillator* oscillator;
};


class AbsDexOsc;
class RelFreqParam;


class AbsFreqParam final : public FrequencyParam {
public:
  AbsFreqParam(BaseOscillator* o, float f);
  void set(float f) override;  // set frequency (propagate to osc and rel freqs)
  float get() override;
  [[nodiscard]] uint32_t get_frequency() const;  // used by rel freq in initial setup
  // on change, frequency is sent to dependent relative freqs
  void add_relative_freq(RelFreqParam* f);
private:
  void set_relative_freqs(uint32_t f) const;
  // we have subtick_bits of fraction so 16 bits is insufficient
  uint32_t frequency;  // TODO - why does this exist separately from the value in the oscillator?
  std::vector<RelFreqParam*> relative_freqs;
};


class RelDexOsc;


class RelFreqParam final : public FrequencyParam {
public:
  class DetuneParam final : public Param {
  public:
    explicit DetuneParam(RelFreqParam* f);
    void set(float val) override;
    float get() override;
  private:
    RelFreqParam* rel_freq_param;
  };
  friend class DetuneParam;
  RelFreqParam(BaseOscillator* o, AbsFreqParam& ref, float r, float d);
  void set(float f) override;  // set ratio
  float get() override;  // get ratio
  void set_root(uint32_t r);
  DetuneParam& get_det_param();  // expose a second param
protected:
  void set_detune(float f);
  float get_detune();
private:
  void recalculate();
  uint32_t root;
  SimpleRatio ratio;
  int16_t detune;
  DetuneParam detune_param;
};


class WavedexMixin {
public:
  class WavedexParam : public Param {
  public:
    WavedexParam(BaseOscillator* o, Wavelib& wl);
    void set(float val) override;
    float get() override;
  private:
    size_t widx;
    BaseOscillator* oscillator;
    Wavelib& wavelib;
  };
  friend class WavedexParam;
  WavedexMixin(BaseOscillator* o, Wavelib& wl);
  WavedexParam& get_dex_param();
protected:
  WavedexParam wavedex;
};


class AbsDexOsc final : public BaseOscillator, public WavedexMixin {
public:
  AbsDexOsc(float f, Wavelib& wl, size_t widx);
  AbsFreqParam& get_freq_param();
private:
  AbsFreqParam freq_param;
};


class RelDexOsc final : public BaseOscillator, public WavedexMixin {
public:
  RelDexOsc(Wavelib& wl, size_t widx, AbsFreqParam& root, float f, float d);
  RelFreqParam& get_freq_param();
private:
  RelFreqParam freq_param;
};


class PolyMixin {
public:
  class CtrlParam : public Param {
  public:
    CtrlParam(float lo, float hi, PolyMixin& m, std::function<bool(float)> s, std::function<float()> g);
    void set(float val) override;
    float get() override;
  private:
    PolyMixin& mixin;
    std::function<bool(float)> delegate_set;
    std::function<float()> delegate_get;
  };
  PolyMixin(BaseOscillator* o, size_t shp, size_t asym, size_t off);
  friend class CtrlParam;
  Param& get_shp_param() const;
  Param& get_asym_param() const;
  Param& get_off_param() const;
protected:
  void update();
private:
  std::unique_ptr<Param> shape_param;
  std::unique_ptr<Param> asym_param;
  std::unique_ptr<Param> offset_param;
  BaseOscillator* oscillator;
  size_t shape;
  size_t asym;
  int offset;
  std::unique_ptr<Wavetable> wtable;
};


class AbsPolyOsc final : public BaseOscillator, public PolyMixin {
public:
  AbsPolyOsc(float f, size_t shp, size_t asym, size_t off);
  AbsFreqParam& get_freq_param();
private:
  AbsFreqParam freq_param;
};


class RelPolyOsc final : public BaseOscillator, public PolyMixin {
public:
  RelPolyOsc(size_t shp, size_t asym, size_t off, AbsFreqParam& root, float f, float d);
  RelFreqParam& get_freq_param();
private:
  RelFreqParam freq_param;
};


#endif
