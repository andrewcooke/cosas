
#ifndef FMCOSA_OSCILLATOR_H
#define FMCOSA_OSCILLATOR_H

#include <array>

#include "constants.h"
#include "source.h"


// since fp is slow we use a lookup table for waveforms.
// eventually we may support multiple waveforms (triangular, square, noise), but for now just sine
// we only store the first quadrant, 16 bits unsigned.

class Wavetable : public Source {

public:

  Wavetable();
  virtual uint16_t next(int64_t tick, int32_t phi) const override;

private:

  std::array<uint16_t, sample_rate / 4> quarter_table;

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
