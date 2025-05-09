
#ifndef FMCOSA_MODEL_H
#define FMCOSA_MODEL_H

import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"


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

  FM(const Source& car, const Source& mod);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Source& carrier;
  const Source& modulator;
  
};
  

class MixedFM : public Source {

public:

  MixedFM(const Source& car, const Source& mod, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const FM fm;
  const Mixer mixer;
  
};
  

class AM : public Source {

public:

  // note that this is not symmetric - src1 is mized against the ring mod output
  AM(const Source& src1, const Source& src2);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Source& source1;
  const Source& source2;
  
};
  

class MixedAM : public Source {

public:

  MixedAM(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const AM am;
  const Mixer mixer;
  
};
  

class Manager {
  
public:
  
  Manager();
  Oscillator& get_oscillator(size_t n) const;
  void set_root(uint16_t freq);
  const Frequency& get_root() const;
  
private:

  vector<unique_ptr<Wavetable>> wavetables;
  vector<unique_ptr<Oscillator>> oscillators;
  void init_wavetables();
  void init_oscillators();
  
};

#endif
