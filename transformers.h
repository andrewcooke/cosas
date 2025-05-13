
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


#endif
