
#ifndef COSA_NODE_H
#define COSA_NODE_H

import std;
using namespace std;

#include "source.h"


// nodes are sources that are connected into an engine.

// at some point they will include support for confuration (and likely
// this will no longer be empty)

class Node : public Source {};


// this is intended to avoid infinite loops in recursive networks

class Latch : Node {

public:

  Latch(Source& s);
  uint16_t next(int64_t tick, int32_t phi) override;

  friend class On;

private:

  Source& source;
  bool on = false;
  uint16_t previous = sample_zero;
  
};


class On {
  
public:
  On(Latch& l);
  ~On();
  
private:
  Latch& latch;

};


#endif
