
#ifndef COSA_OSCILLATOR_H
#define COSA_OSCILLATOR_H

#include <memory>
#include <functional>

#include "cosas/maths.h"
#include "cosas/node.h"
#include "cosas/params.h"
#include "cosas/wavelib.h"


class WavdexMixin;
class PolyMixin;


class BaseOscillator : public Node {

public:

  friend class PolyMixin;
  friend class Frequency;
  friend class WavedexMixin;
  BaseOscillator(Wavetable *t);
  int16_t next(int32_t tick, int32_t phi) const override;

protected:

  uint32_t frequency;
  Wavetable* wavetable;
  
};


class Frequency : public Param {

public:

  Frequency(BaseOscillator* o);
  
protected:

  // on change, frequency is sent to controlled oscillator
  void set_oscillator(uint32_t f);
  BaseOscillator* oscillator;
  
};


class AbsDexOsc;
class RelativeFreq;


class AbsoluteFreq : public Frequency {
  
public:

  AbsoluteFreq(BaseOscillator* o, float freq);
  void set(float f) override;  // set frequency (propagate to osc and rel freqs)
  uint32_t get_frequency() const;  // used by rel freq in initial setup
  // on change, frequency is sent to dependent relative freqs
  void add_relative_freq(RelativeFreq* f);
  
private:

  void set_relative_freqs(uint32_t f);

  // we have subtick_bits of fraction so 16 bits is insufficient
  uint32_t frequency;
  std::vector<RelativeFreq*> relative_freqs; 

};


class RelDexOsc;


class RelativeFreq : public Frequency {
  
public:

  class Detune : public Param {
  public:
    Detune(RelativeFreq* f);
    void set(float val) override;
  private:
    RelativeFreq* frequency;
  };

  RelativeFreq(RelDexOsc* o, AbsoluteFreq& ref, float r, float d);
  void set(float f) override;  // set ratio
  void set_detune(float f);
  void set_root(uint32_t);
  Detune& get_det();  // expose a second param

  //  friend class Detune;
  //  friend class AbsoluteFreq;
  
private:

  void recalculate();

  uint32_t root;
  SimpleRatio ratio;
  int16_t detune;
  Detune detune_param;
  
};


class WavedexMixin {

public:

  class Wavedex : public Param {
  public:
    Wavedex(BaseOscillator* o, Wavelib& wl);
    void set(float val) override;
  private:
    BaseOscillator* oscillator;
    Wavelib& wavelib;
  };

  friend class Wavedex;
  WavedexMixin(BaseOscillator* o, Wavelib& wl);
  Wavedex& get_dex();

protected:

  Wavedex wavedex;
  
};


class AbsDexOsc : public BaseOscillator, public WavedexMixin {

public:

  // TODO - f first
  AbsDexOsc(Wavelib& wl, size_t widx, float f);
  AbsoluteFreq& get_freq();

private:

  AbsoluteFreq freq_param;
  
};


class RelDexOsc : public BaseOscillator, public WavedexMixin {

public:

  RelDexOsc(Wavelib& wl, size_t widx, AbsoluteFreq& root, float f, float d);
  RelativeFreq& get_freq();
  
private:

  RelativeFreq freq_param;
  
};


class PolyMixin {

public:

  class Ctrl : public Param {
  public:
    Ctrl(PolyMixin& m, std::function<void(float)> d);
    void set(float val) override;
  private:
    PolyMixin& mixin;
    std::function<void(float)> delegate;
    
  };

  PolyMixin(BaseOscillator* o, size_t shp, size_t asym, size_t off);
  friend class Ctrl;
  Param& get_shp();
  Param& get_asym();
  Param& get_off();

protected:
  
  void update();
  
private:

  std::unique_ptr<Ctrl> p_shape;
  std::unique_ptr<Ctrl> p_asym;
  std::unique_ptr<Ctrl> p_offset;
  BaseOscillator* oscillator;
  float shape;
  float asym;
  float offset;
  std::unique_ptr<Wavetable> unq_wtable;
  
};


class AbsPolyOsc : public BaseOscillator, public PolyMixin {

public:

  AbsPolyOsc(float f, size_t shp, size_t asyn, size_t off);
  AbsoluteFreq& get_freq();

private:

  AbsoluteFreq freq_param;
  
};


#endif
