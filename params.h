
#ifndef COSA_PARAMS_H
#define COSA_PARAMS_H

import std;
using namespace std;

#include "constants.h"
#include "maths.h"


// sub-oscillators run at frequencies that are multiples of the main oscillator.
// this class encapsulates that scaling.
// it's non-trivial because we want fractional scaling without division.
// so we support division by powers of 2 (bit shifts) and, as special cases, 3 and 5
// (chosen so that we can handle major and minor chords).

// alternatively, maybe we do want exact frequencies for dissonance, etc.
// so support that too

class Frequency {

public:
  
  virtual uint16_t get() const = 0;

};


class AbsoluteFreq : public Frequency {
  
public:

  AbsoluteFreq(uint16_t freq);
  uint16_t get() const override;
  void set(uint16_t freq);

private:

  uint16_t frequency;

};


// this could be simplified maybe by keeping an instance of the
// SimpleRatio rather than a smart pointer.

class RelativeFreq : public Frequency {
  
public:

  // care must be taken for arg 1 to eventually bottom out with an AbsoluteFreq
  RelativeFreq(const Frequency& ref, unique_ptr<SimpleRatio> r);
  RelativeFreq(const Frequency& ref, float r, bool ext);
  uint16_t get() const override;

private:

  const Frequency& reference;
  unique_ptr<SimpleRatio> ratio;
  
};


class Amplitude {

public:

  const float factor;
  
  Amplitude(float factor);
  uint16_t scale(uint16_t amp) const;
  
private:

  uint16_t norm;
  
};

const auto zero_amp = Amplitude(0);
const auto unit_amp = Amplitude(1);


class Balance {

public:

  const float wet;
  
  Balance(float wet);
  uint16_t combine(uint16_t wet, uint16_t dry) const;

private:

  uint16_t wet_weight, dry_weight;
  
};

const auto dry_bal = Balance(0);
const auto wet_bal = Balance(1);


#endif
