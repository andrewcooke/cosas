
#ifndef COSA_OSCILLATOR_H
#define COSA_OSCILLATOR_H

#include <memory>
#include <functional>

#include "cosas/maths.h"
#include "cosas/node.h"
#include "cosas/params.h"
#include "cosas/wavelib.h"


// looks up the waveform in a wavetable, given the frequency
class BaseOscillator : public RelSource {
public:
  friend class PolyMixin;
  friend class FrequencyParam;
  friend class WavedexMixin;
  BaseOscillator(uint32_t f, Wavetable *t);
  [[nodiscard]] int16_t next(int32_t tick, int32_t phi) const override;
protected:
  uint32_t frequency;  // subtick units
  Wavetable* wavetable;
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
  [[nodiscard]] uint32_t get_frequency() const;  // used by rel freq in initial setup
  // on change, frequency is sent to dependent relative freqs
  void add_relative_freq(RelFreqParam* f);
private:
  void set_relative_freqs(uint32_t f) const;
  // we have subtick_bits of fraction so 16 bits is insufficient
  uint32_t frequency;
  std::vector<RelFreqParam*> relative_freqs;
};


class RelDexOsc;


class RelFreqParam final : public FrequencyParam {
public:
  class DetuneParam final : public Param {
  public:
    explicit DetuneParam(RelFreqParam* f);
    void set(float val) override;
  private:
    RelFreqParam* rel_freq_param;
  };
  friend class DetuneParam;
  RelFreqParam(BaseOscillator* o, AbsFreqParam& ref, float r, float d);
  void set(float f) override;  // set ratio
  void set_root(uint32_t r);
  DetuneParam& get_det_param();  // expose a second param
protected:
  void set_detune(float f);
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
  private:
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
    CtrlParam(PolyMixin& m, std::function<void(float)> d);
    void set(float val) override;
  private:
    PolyMixin& mixin;
    std::function<void(float)> delegate;
  };
  PolyMixin(BaseOscillator* o, size_t shp, size_t asym, size_t off);
  friend class CtrlParam;
  Param& get_shp_param() const;
  Param& get_asym_param() const;
  Param& get_off_param() const;
protected:
  void update();
private:
  std::unique_ptr<CtrlParam> shape_param;
  std::unique_ptr<CtrlParam> asym_param;
  std::unique_ptr<CtrlParam> offset_param;
  BaseOscillator* oscillator;
  float shape;
  float asym;
  float offset;
  std::unique_ptr<Wavetable> wtable;
};


class AbsPolyOsc final : public BaseOscillator, public PolyMixin {
public:
  AbsPolyOsc(float f, size_t shp, size_t asyn, size_t off);
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
