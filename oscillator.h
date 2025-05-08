
#ifndef FMCOSA_OSCILLATOR_H
#define FMCOSA_OSCILLATOR_H

import std;
using namespace std;

#include "constants.h"
#include "source.h"


// since fp is slow we use a lookup table for waveforms.
// eventually we may support multiple waveforms (triangular, square, noise), but for now just sine
// we only store the first quadrant, 16 bits unsigned.

class Wavetable : public Source {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  uint16_t raw(size_t i) const {return quarter_table.at(i);}
  
protected:

  array<uint16_t, sample_rate / 4> quarter_table;

};


class Sine : public Wavetable {

public:

  Sine();

};


class Square : public Wavetable {

public:

  Square();

};


class Triangle : public Wavetable {

public:

  Triangle();

};


class InterpWtable : public Wavetable {

public:

  InterpWtable(Wavetable& wtable1, Wavetable& wtable2, float weight1);

};


// it turns out that a wavetable isn't a good abstraction for a source.
// it's better to bundle a wavetable with an amplitude and frequency, where the frequecy can be scaled.

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
