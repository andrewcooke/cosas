
#ifndef FMCOSA_MODEL_H
#define FMCOSA_MODEL_H

import std;
using namespace std;

#include "constants.h"
#include "source.h"


class Balance {

public:

  const float wet;
  
  Balance(float wet);
  uint16_t combine(uint16_t wet, uint16_t dry);

private:

  uint16_t wet_weight, dry_weight;
  
};


class Mixer : public Source {
    
public:
    
  Mixer(Source src1, Source src2, AmpScale vol, Balance bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  Source source1;
  Source source2;
  AmpScale volume;
  Balance balance;
  
};


class FM : public Source {

public:

  FM(Source car, Source mod, AmpScale vol, Balance bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  Source carrier;
  Source modulation;
  Mixer mixer;
  
};
  

class AM : public Source {

public:

  AM(Source car, Source mod, AmpScale vol, Balance bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  Source carrier;
  Source modulation;
  Mixer mixer;
  
};
  

#endif

