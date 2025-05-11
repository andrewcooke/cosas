
#ifndef COSA_MODULATORS_H
#define COSA_MODULATORS_H

import std;
using namespace std;

#include "constants.h"
#include "params.h"
#include "node.h"


class Mixer : public Node {
    
public:
    
  Mixer(const Node& nd1, const Node& nd2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Node& node1;
  const Node& node2;
  const Amplitude& amplitude;
  const Balance& balance;
  
};


class FM : public Node {

public:

  FM(const Node& car, const Node& mod);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Node& carrier;
  const Node& modulator;
  
};
  

class MixedFM : public Node {

public:

  MixedFM(const Node& car, const Node& mod, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const FM fm;
  const Mixer mixer;
  
};
  

class AM : public Node {

public:

  // note that this is not symmetric - nd1 is mized against the ring mod output
  AM(const Node& nd1, const Node& nd2);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const Node& node1;
  const Node& node2;
  
};
  

class MixedAM : public Node {

public:

  MixedAM(const Node& nd1, const Node& nd2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) const override;

private:

  const AM am;
  const Mixer mixer;
  
};
  

#endif
