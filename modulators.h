
#ifndef COSA_MODULATORS_H
#define COSA_MODULATORS_H

#include "constants.h"
#include "params.h"
#include "transformers.h"
#include "node.h"


// constructor takes two nodes

class Modulator : public Node {};


class Merge : public Modulator {
    
public:
    
  Merge(const Node& w, const Node& d, Balance& bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const Node& wet;
  const Node& dry;
  Balance& balance;
  
};


class Mixer : public Modulator {
    
public:
    
  Mixer(const Node& nd1, const Node& nd2, Amplitude& amp, Balance& bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const Node& node1;
  const Node& node2;
  Amplitude& amplitude;
  Balance& balance;
  
};


class FM : public Modulator {

public:

  FM(const Node& car, const Node& mod);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const Node& carrier;
  const Node& modulator;
  
};
  

class AM : public Modulator {

public:

  // note that this is not symmetric - nd1 is mized against the ring mod output
  AM(const Node& nd1, const Node& nd2);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const Node& node1;
  const Node& node2;
  
};
  

#endif
