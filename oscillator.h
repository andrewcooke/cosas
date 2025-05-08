
#ifndef FMCOSA_OSCILLATOR_H
#define FMCOSA_OSCILLATOR_H

import std;
using namespace std;

#include "constants.h"
#include "source.h"


// since fp is slow we use lookup tables for waveforms.
// note that we can use fp to generate tables because it's done up-front (even, we could load from disc).

class Wavetable : public Source {

public:
  
  virtual uint16_t raw(size_t i) const = 0;
  
};


class QuarterWtable : public Wavetable {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  uint16_t raw(size_t i) const {return quarter_table.at(i);}
  
protected:

  array<uint16_t, sample_rate / 4> quarter_table;

};


class Sine : public QuarterWtable {

public:

  Sine();

};


class Square : public QuarterWtable {

public:

  Square();

};


class Triangle : public QuarterWtable {

public:

  Triangle();

};


class InterpQWtable : public QuarterWtable {

public:

  InterpQWtable(QuarterWtable& wtable1, QuarterWtable& wtable2, float weight1);

};


class HalfWtable : public Wavetable {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  uint16_t raw(size_t i) const {return half_table.at(i);}
  
protected:

  array<uint16_t, sample_rate / 2> half_table;

};


class Saw : public HalfWtable {

public:

  // a regular saw can be done with a quarter wavetable, but with a different unpacking logic.
  // for full generalility, however, we need half (and even then it's complex).
  // offset from -1 to 1.
  Saw(float offset);

};


class FullWtable : public Wavetable {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  uint16_t raw(size_t i) const {return full_table.at(i);}
  
protected:

  array<uint16_t, sample_rate> full_table;

};


class WhiteNoise : public FullWtable {

public:

  WhiteNoise();

};


// it turns out that a wavetable isn't a good abstraction for a source.
// it's better to bundle a wavetable with an amplitude and frequency.

class Oscillator : public Source {

public:

  Oscillator(const Wavetable& wave, const Amplitude& amp, uint16_t freq, const Multiplier& mult);
  Oscillator(const Wavetable& wave, const Amplitude& amp, uint16_t fund);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:
  const Wavetable& wavetable;
  const Amplitude& amplitude;
  uint16_t frequency;
  const Multiplier& multiplier;
  
};

#endif
