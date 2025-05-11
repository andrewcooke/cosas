
#ifndef COSA_OSCILLATOR_H
#define COSA_OSCILLATOR_H

import std;
using namespace std;

#include "constants.h"
#include "wavetables.h"
#include "params.h"
#include "node.h"


// foward decl
class Manager;


// a wavetable isn't a very practical abstraction for a source.  it's
// better to bundle a wavetable with an amplitude and frequency.

class Oscillator : public Node {

public:

  Oscillator(Wavetable& wave, unique_ptr<Frequency> freq);
  Oscillator(Wavetable& wave, unique_ptr<Frequency> freq, unique_ptr<Amplitude> amp);
  uint16_t next(int64_t tick, int32_t phi) const override;
  void set_freq_abs(uint16_t freq);
  void set_freq_ratio(const Manager&, float ratio);
  const Frequency& get_frequency();
  
private:
  Wavetable& wavetable;
  unique_ptr<Frequency> frequency;
  unique_ptr<Amplitude> amplitude;
  
};

#endif
