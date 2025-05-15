
#ifndef COSA_OSCILLATOR_H
#define COSA_OSCILLATOR_H

#include <memory>

#include "constants.h"
#include "lookup.h"
#include "params.h"
#include "node.h"


// foward decl
class Manager;


// a wavetable isn't a very practical abstraction for a source.  it's
// better to bundle a wavetable with an amplitude and frequency.

class Oscillator : public Node {

public:

  Oscillator(Wavetable& wave, std::unique_ptr<Frequency> freq);
  Oscillator(Wavetable& wave, std::unique_ptr<Frequency> freq, Amplitude amp);
  int16_t next(int64_t tick, int32_t phi) override;
  const Frequency& get_frequency();
  
private:
  Wavetable& wavetable;
  std::unique_ptr<Frequency> frequency;
  Amplitude amplitude;
  
};

#endif
