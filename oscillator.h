
#ifndef COSA_OSCILLATOR_H
#define COSA_OSCILLATOR_H

#include <memory>

#include "maths.h"
#include "node.h"
#include "params.h"
#include "wavelib.h"

class Oscillator : public Node {

public:

  class Wavedex : public Param {
  public:
    Wavedex(Oscillator* o, Wavelib& wl);
    void set(float val) override;
  private:
    Oscillator* oscillator;
    Wavelib& wavelib;
  };

  Oscillator(Wavelib& w, size_t widx);
  friend class Frequency;
  friend class Wavedex;
  Wavedex& get_wavedex();
  int16_t next(int32_t tick, int32_t phi) const override;

protected:

  Wavedex wavedex;
  uint32_t frequency;
  Wavetable* wavetable;
  
};


class Frequency : public Param {

public:

  Frequency(Oscillator* o);
  
protected:

  // on change, frequency is sent to controlled oscillator
  void set_oscillator(uint32_t f);
  Oscillator* oscillator;
  
};


class AbsoluteOsc;
class RelativeFreq;


class AbsoluteFreq : public Frequency {
  
public:

  AbsoluteFreq(AbsoluteOsc* o, float freq);
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


class RelativeOsc;


class RelativeFreq : public Frequency {
  
public:

  class Detune : public Param {
  public:
    Detune(RelativeFreq* f);
    void set(float val) override;
  private:
    RelativeFreq* frequency;
  };

  RelativeFreq(RelativeOsc* o, AbsoluteFreq& ref, float r, float d);
  void set(float f) override;  // set ratio
  void set_detune(float f);
  void set_root(uint32_t);
  Detune& get_detune();  // expose a second param

  //  friend class Detune;
  //  friend class AbsoluteFreq;
  
private:

  void recalculate();

  uint32_t root;
  SimpleRatio ratio;
  int16_t detune;
  Detune detune_param;
  
};


class AbsoluteOsc : public Oscillator {

public:

  AbsoluteOsc(Wavelib& wl, size_t widx, float f);
  AbsoluteFreq& get_param();

private:

  AbsoluteFreq freq_param;
  
};


class RelativeOsc : public Oscillator {

public:

  RelativeOsc(Wavelib& wl, size_t widx, AbsoluteFreq& root, float f, float d);
  RelativeFreq& get_param();
  
private:

  RelativeFreq freq_param;
  
};


#endif
