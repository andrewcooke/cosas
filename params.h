
#ifndef COSA_PARAMS_H
#define COSA_PARAMS_H

#include "wavelib.h"
#include "constants.h"
#include "maths.h"


// these are small enough to be treated as values without worrying
// about ppinters and references.

// sub-oscillators run at frequencies that are multiples of the main
// oscillator.  this class encapsulates that scaling.  it's
// non-trivial because we want fractional scaling without division.
// so we support division by powers of 2 (bit shifts) and, as special
// cases, 3 and 5 (chosen so that we can handle major and minor
// chords).

// alternatively, maybe we do want exact frequencies for dissonance,
// etc.  so support that too


class Frequency {

public:

  virtual uint32_t get_frequency() const = 0;

};


class AbsoluteFreq : public Frequency {
  
public:

  AbsoluteFreq(float freq);
  uint32_t get_frequency() const override;
  
  void set_frequency(float f);

private:

  // we have subtick_bits of fraction so 16 bits is insufficient
  uint32_t frequency;

};


class RelativeFreq : public Frequency {
  
public:

  RelativeFreq(Frequency& ref, SimpleRatio r, float d);
  RelativeFreq(Frequency& ref, float r, float d);
  RelativeFreq(Frequency& ref, SimpleRatio r);
  RelativeFreq(Frequency& ref, float r);
  uint32_t get_frequency() const override;

  void set_ratio(float r);
  void set_detune(float f);
  
private:

  Frequency& reference;
  SimpleRatio ratio;
  uint16_t detune;
  
};


class Amplitude {

public:

  Amplitude(float f);
  Amplitude();  // full on
  int16_t scale(int16_t amp) const;

  void set_amplitude(float a);
  
private:

  // could this be faster as an int mult and shift?
  float amplitude;
  
};

const auto zero_amp = Amplitude(0);
const auto unit_amp = Amplitude(1);


class Balance {

public:

  Balance();  // full wet
  Balance(float w);
  int16_t combine(int16_t wet, int16_t dry) const;

  void set_wet_weight(float w);

private:

  // could this be faster as ints and shift?
  float wet_weight;
  
};

const auto dry_bal = Balance(0);
const auto wet_bal = Balance(1);


class Wavedex {

public:

  Wavedex(Wavelib& wl, size_t idx);
  Wavetable& get_wavetable() const;

  void set_wavedex(float idx);

private:

  Wavelib& wavelib;
  Wavetable& wavetable;  // cached
  size_t wavedex;
  
};

#endif
