
#ifndef FMCOSA_OSCILLATOR_H
#define FMCOSA_OSCILLATOR_H

import std;
using namespace std;

#include "constants.h"
#include "source.h"


// since fp is slow we generally use lookup tables for waveforms.
// note that we can use fp to generate tables because it's done up-front (even, we could load from disc).

class Wavetable : public Source {};


class Square : public Wavetable {

public:

  Square() : Square(0.5) {};
  Square(float duty);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  size_t duty_idx;
  
};


class QuarterWtable : public Wavetable {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  
protected:

  array<uint16_t, sample_rate / 4> quarter_table;

};


class Sine : public QuarterWtable {

public:

  Sine() : Sine(1) {};
  Sine(float gamma);

};


class Triangle : public QuarterWtable {

public:

  Triangle();

};


class HalfWtable : public Wavetable {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  
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


class FullWtable : public Source {

public:
  
  uint16_t next(int64_t tick, int32_t phi) const override;
  
protected:

  array<uint16_t, sample_rate> full_table;

};


class Noise : public FullWtable {

public:

  Noise() : Noise(1) {};
  Noise(int smooth);

};


// it turns out that a wavetable isn't a very practical abstraction for a source.
// it's better to bundle a wavetable with an amplitude and frequency.

class Oscillator : public Wavetable {

public:

  Oscillator(Wavetable& wave, unique_ptr<Amplitude> amp, unique_ptr<Frequency> freq);
  uint16_t next(int64_t tick, int32_t phi) const override;
  void set_freq_abs(uint16_t freq);
  void set_freq_ratio(const AbsoluteFreq& root, float ratio);
  const Frequency& get_frequency();
  
private:
  Wavetable& wavetable;
  unique_ptr<Amplitude> amplitude;
  unique_ptr<Frequency> frequency;
  
};

#endif
