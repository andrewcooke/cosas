
#ifndef COSA_NODE_H
#define COSA_NODE_H

#include <initializer_list>
#include <list>
#include <memory>

#include "source.h"


// nodes are sources that are connected into an engine.

// at some point they will include support for confuration (and likely
// this will no longer be empty)

class Node : public Source {};


// useful for testing

class Constant : public Node {

public:

  Constant(int16_t v);
  int16_t next(int32_t tick, int32_t phi) override;

private:

  int16_t value;
  
};

extern Constant zero;


class Sequence : public Node {

public:

  Sequence(std::initializer_list<int16_t> vs);
  int16_t next(int32_t tick, int32_t phi) override;

private:

  std::unique_ptr<std::list<int16_t>> values;
  
};


// this is intended to avoid infinite loops in recursive networks.
// the setter also allows loops to be constructed.

class Latch : public Node {

public:

  Latch();
  void set_source(Source& s);
  int16_t next(int32_t tick, int32_t phi) override;

  friend class On;

private:

  Source& source;
  bool on = false;
  int16_t previous = 0;
  
};


class On {
  
public:

  On(Latch& l);
  ~On();
  
private:

  Latch& latch;

};


#endif
