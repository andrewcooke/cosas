
#ifndef COSA_MODULATORS_H
#define COSA_MODULATORS_H

#include "constants.h"
#include "params.h"
#include "transformers.h"
#include "node.h"


// various combinations; typically with gain merged or not

class Modulator : public Node {};


class Merge : public Modulator {
    
public:
    
  Merge(Node& w, Node& d, Balance bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  Node& wet;
  Node& dry;
  Balance balance;
  
};


class Mixer : public Modulator {
    
public:
    
  Mixer(Node& nd1, Node& nd2, Amplitude amp, Balance bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  Node& node1;
  Node& node2;
  Amplitude amplitude;
  Balance balance;
  
};


class FM : public Modulator {

public:

  FM(Node& car, Node& mod);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  Node& carrier;
  Node& modulator;
  
};
  

class MixedFM : public Modulator {

public:

  MixedFM(Node& car, Node& mod, Amplitude amp, Balance bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  FM fm;
  Mixer mixer;
  
};


// compared to MixedFM, this has the gain on the modulator

class ModularFM : public Modulator {

public:

  ModularFM(Node& car, Node& mod, Amplitude amp, Balance bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  FM fm;
  Gain gain;
  Merge merge;
  
};



class AM : public Modulator {

public:

  // note that this is not symmetric - nd1 is mized against the ring mod output
  AM(Node& nd1, Node& nd2);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  Node& node1;
  Node& node2;
  
};
  

class MixedAM : public Modulator {

public:

  MixedAM(Node& nd1, Node& nd2, Amplitude amp, Balance bal);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  AM am;
  Mixer mixer;
  
};
  

#endif
