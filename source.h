
#ifndef FMCOSA_SOURCE_H
#define FMCOSA_SOURCE_H

#include "constants.h"


// we measure time in ticks, one tick every 1/44100 s.
// the wavetable (unpacked to four quadrants) has a 44100 samples per wavelength.
// to get a 440Hz tone we need to output the entire wavelength in 1/440 s.
// since we output a sample every 1/44100 s we need to generate (1/440) / (1/44100) = 44100 / 440 =~ 100 samples.
// so we need to advance 44100 / 100 =~ 440 indices through the table on each tick.
// an octave higher, at 880Hz, we need to output the entire wavelength in half that time so we need to advance twice as fast, advancing ~880 indices each tick.
// typically we talk about sine(omega t + phi); here we are doing wavetable[omega tick] where omega is the number of indices per tick.
// for a 1Hz tone we need omega is 1, because we cover a whole cycle in 44100 ticks (when omega x tick = 44100).
// in short, and exactly, omega is f.
// this isn't by chance, it's because the wavetable has exactly the number of entries needed to "last" for one second.

// for fm modulation we need to evaluate sin(omega t + phi)
// this translates into wavetable[f tick + phi] in our units.
// note that phi is independent of f, so we it is not simply a scaling of time/tick.
// so we need both tick and phi in our interface below.

class Source {

 public:

  // need to be signed because phi can be negative and we need to add the two
  virtual uint16_t next(int64_t tick, int32_t phi) const = 0;
  
};


// sub-oscillators run at frequencies that are multiples of the main oscillator.
// this class encapsulates that scaling.
// it's non-trivial because we want fractional scaling without division.
// so we support division by powers of 2 (bit shifts) and, as special cases, 3 and 5
// (chosen so that we can handle major and minor chords).

class Multiplier {

public:

  const uint16_t numerator;
  const uint16_t denominator;

  Multiplier(uint16_t n, uint16_t d);
  uint16_t scale(uint16_t freq) const;

private:

  uint8_t bits;
  bool three;
  bool five;

};

const auto unit_mult = Multiplier(1, 1);


uint16_t clip(uint32_t inter);
uint16_t clip(int32_t inter);
uint16_t clip(float inter);


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
