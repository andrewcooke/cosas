
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

  Constant(const int16_t v);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const int16_t value;
  
};

extern Constant zero;


class Sequence : public Node {

public:

  Sequence(std::initializer_list<int16_t> vs);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  std::unique_ptr<std::list<int16_t>> values;
  
};


// this is intended to avoid infinite loops in recursive networks.
// the setter also allows loops to be constructed.

// although this is very obviously mutable it is all declared const
// and implemented with mutable to avoid polluting the Node/Source
// interface.

class Latch : public Node {

public:

  Latch();
  void set_source(const Source* s) const;
  int16_t next(int32_t tick, int32_t phi) const override;

  friend class SetOnInScope;

private:

  mutable const Source* source;
  mutable bool on = false;
  mutable int16_t previous = 0;
  
};


class SetOnInScope {
  
public:

  SetOnInScope(const Latch* l);
  ~SetOnInScope();
  
private:

  const Latch* latch;

};


#endif
