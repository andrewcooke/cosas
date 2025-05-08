
#ifndef FMCOSA_MODEL_H
#define FMCOSA_MODEL_H

#include "constants.h"
#include "source.h"


class Mixer : public Source {
    
public:
    
  Mixer(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Source& source1;
  const Source& source2;
  const Amplitude& amplitude;
  const Balance& balance;
  
};


class FM : public Source {

public:

  FM(const Source& car, const Source& mod, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  class FMImpl;
  const FMImpl& fm;
  const Mixer& mixer;
  
};
  

class AM : public Source {

public:

  // note that this is not symmetric - src1 is mized against the ring mod output
  AM(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  class AMImpl;
  const AMImpl& am;
  const Mixer& mixer;
  
};
  

#endif

