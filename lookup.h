
#ifndef COSA_LOOKUP_H
#define COSA_LOOKUP_H

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
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  size_t duty_idx;
  
};


class QuarterWtable : public Wavetable {

public:
  
  uint16_t next(int64_t tick, int32_t phi) override;
  
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
  
  uint16_t next(int64_t tick, int32_t phi) override;
  
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
  
  uint16_t next(int64_t tick, int32_t phi) override;
  
protected:

  array<uint16_t, sample_rate> full_table;

};


class Noise : public FullWtable {

public:

  Noise() : Noise(1) {};
  Noise(int smooth);

};


// similar to wavetables, but not Source instances, transfer functions
// map half-samples (+ve going).  classes that use this handle the
// symmetric (or not) treatment of negative going.

class Transfer {

public:

  virtual uint8_t lookup(uint8_t sample) const;

};


class TransferFn : public Transfer {

public:
  
  TransferFn();
  uint8_t lookup(uint8_t sample) const override;
  virtual void init_table();

protected:

  unique_ptr<array<uint8_t, full_lookup_size>> table = make_unique<array<uint8_t, full_lookup_size>>();

};


class Gamma : public TransferFn {

public:

  Gamma(float g);
  void init_table() override;

private:

  float gamma;

};

#endif
