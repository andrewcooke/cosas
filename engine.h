
#ifndef FMCOSA_MODEL_H
#define FMCOSA_MODEL_H

import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"


class Node : public Source {};


class Mixer : public Node {
    
public:
    
  Mixer(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Source& source1;
  const Source& source2;
  const Amplitude& amplitude;
  const Balance& balance;
  
};


class FM : public Node {

public:

  FM(const Source& car, const Source& mod);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Source& carrier;
  const Source& modulator;
  
};
  

class MixedFM : public Node {

public:

  MixedFM(const Source& car, const Source& mod, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const FM fm;
  const Mixer mixer;
  
};
  

class AM : public Node {

public:

  // note that this is not symmetric - src1 is mized against the ring mod output
  AM(const Source& src1, const Source& src2);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Source& source1;
  const Source& source2;
  
};
  

class MixedAM : public Node {

public:

  MixedAM(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const AM am;
  const Mixer mixer;
  
};
  

class Manager {
  
public:
  
  Manager(bool ext);
  Oscillator& get_oscillator(size_t n) const;
  void set_root(uint16_t freq);
  const AbsoluteFreq& get_root() const;
  bool is_extended() const;
  vector<Source&> build_model(size_t n);

  const size_t n_models = 0;
    
private:

  bool extended;
  AbsoluteFreq* root;
  vector<unique_ptr<Wavetable>> wavetables;
  void init_wavetables();
  vector<unique_ptr<Oscillator>> oscillators;
  void init_oscillators();
  vector<unique_ptr<Node>> nodes;
          
};

#endif
