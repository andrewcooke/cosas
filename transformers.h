
#ifndef COSA_TRANSFORMERS_H
#define COSA_TRANSFORMERS_H

import std;
using namespace std;

#include "constants.h"
#include "params.h"
#include "node.h"
#include "lookup.h"


// these have one input (modulators have two)


class Transformer : public Node {

protected:

  Transformer(Node& nd) : node(nd) {};
  Node& node;
  
};


class Gain : public Transformer {
    
public:
    
  Gain(Node& nd, const Amplitude& amp);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  const Amplitude& amplitude;
  
};

/*
class Functional : public Transformer {

public:

  Functional(Node& nd, float k);
  uint16_t next(int64_t tick, int32_t phi) override;
  virtual float func(float k, float x);
  
private:

  float constant;
  
};
*/

class Compander : public Transformer {

public:

  Compander(Node& nd, float g);
  uint16_t next(int64_t tick, int32_t phi) override;

private:

  float gamma;
  
};


#endif
