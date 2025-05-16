
#ifndef COSA_PARAMS_H
#define COSA_PARAMS_H

#include "constants.h"
#include "maths.h"


// generally these are small enough to be treated as values without
// worrying about piinters and references.

// sub-oscillators run at frequencies that are multiples of the main
// oscillator.  this class encapsulates that scaling.  it's
// non-trivial because we want fractional scaling without division.
// so we support division by powers of 2 (bit shifts) and, as special
// cases, 3 and 5 (chosen so that we can handle major and minor
// chords).

// alternatively, maybe we do want exact frequencies for dissonance,
// etc.  so support that too

class AbsoluteFreq;  // forwards decl


class Frequency {

public:

  virtual AbsoluteFreq& get_root() = 0;
  virtual uint16_t get() const = 0;

};


class AbsoluteFreq : public Frequency {
  
public:

  AbsoluteFreq(uint16_t freq);
  AbsoluteFreq& get_root() override;
  uint16_t get() const override;

private:

  uint16_t frequency;

};


class RelativeFreq : public Frequency {
  
public:

  RelativeFreq(Frequency& ref, SimpleRatio r);
  RelativeFreq(Frequency& ref, float r);
  AbsoluteFreq& get_root() override;
  uint16_t get() const override;

private:

  Frequency& reference;
  SimpleRatio ratio;
  
};


class Amplitude {

public:

  const float factor;
  
  Amplitude();  // full on
  Amplitude(float factor);
  int16_t scale(int16_t amp) const;
  
private:

  uint16_t norm;
  
};

const auto zero_amp = Amplitude(0);
const auto unit_amp = Amplitude(1);


class Balance {

public:

  const float wet;

  Balance();  // full wet
  Balance(float wet);
  int16_t combine(int16_t wet, int16_t dry) const;

private:

  uint16_t wet_weight, dry_weight;
  
};

const auto dry_bal = Balance(0);
const auto wet_bal = Balance(1);


#endif
