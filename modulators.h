
#ifndef COSA_MODULATORS_H
#define COSA_MODULATORS_H

import std;
using namespace std;

#include "constants.h"
#include "params.h"
#include "node.h"


class Mixer : public Node {
    
public:
    
  Mixer(Node& nd1, Node& nd2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  Node& node1;
  Node& node2;
  const Amplitude& amplitude;
  const Balance& balance;
  
};


class FM : public Node {

public:

  FM(Node& car, Node& mod);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  Node& carrier;
  Node& modulator;
  
};
  

class MixedFM : public Node {

public:

  MixedFM(Node& car, Node& mod, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  FM fm;
  Mixer mixer;
  
};
  

class AM : public Node {

public:

  // note that this is not symmetric - nd1 is mized against the ring mod output
  AM(Node& nd1, Node& nd2);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  Node& node1;
  Node& node2;
  
};
  

class MixedAM : public Node {

public:

  MixedAM(Node& nd1, Node& nd2, const Amplitude& amp, const Balance& bal);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  AM am;
  Mixer mixer;
  
};
  

#endif
