
#ifndef COSA_PARAMS_H
#define COSA_PARAMS_H

#include "control.h"
#include "wavetable.h"
#include "wavelib.h"


class Param : public Input {};


class Amplitude : public Param {

public:

  Amplitude(float f);
  Amplitude();  // full on
  int16_t scale(int16_t amp) const;
  void set(float f) override;

private:

  // could this be faster as an int mult and shift?
  float amplitude;
  
};

const auto zero_amp = Amplitude(0);
const auto unit_amp = Amplitude(1);


class Balance : public Param {

public:

  Balance();  // full wet
  Balance(float w);
  int16_t combine(int16_t wet, int16_t dry) const;
  void set(float /* f */) override {};

private:

  // could this be faster as ints and shift?
  float wet_weight;
  
};

const auto dry_bal = Balance(0);
const auto wet_bal = Balance(1);


#endif
