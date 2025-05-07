
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

  Wavetable();
  virtual uint16_t next(int64_t tick, int32_t phi) const override;
  uint16_t at_uint16_t(uint64_t tick, StepScale scale) const;
  float at_float(uint64_t tick, StepScale scale) const;

private:

  array<uint16_t, table_size> quarter_table;

};


// it turns out that a wavetable isn't a good abstraction for a source.
// it's better to bundle a wavetable with an amplitude and frequency, where the frequecy can be scaled.

class Oscillator : public Source {

public:

  Oscillator(Wavetable wave, AmpScale vol, uint16_t freq, Multiplier mult);
  Oscillator(Wavetable wave, AmpScale vol, uint16_t fund);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:
  Wavetable wavetable;
  AmpScale volume;
  uint16_t frequency;
  Multiplier multiplier;
  
}

#endif
