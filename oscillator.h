
#ifndef COSA_OSCILLATOR_H
#define COSA_OSCILLATOR_H

#include <memory>

#include "constants.h"
#include "params.h"
#include "node.h"


// foward decl
class Manager;


// a wavetable isn't a very practical abstraction for a source.  it's
// better to bundle a wavetable with an amplitude and frequency.

class Oscillator : public Node {

public:

  Oscillator(Wavedex w, std::unique_ptr<Frequency> f);
  int16_t next(int32_t tick, int32_t phi) const override;
  
private:
  Wavedex wavedex;
  std::unique_ptr<Frequency> frequency;
  
};


#endif
