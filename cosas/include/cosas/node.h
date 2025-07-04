
#ifndef COSAS_NODE_H
#define COSAS_NODE_H

#include <initializer_list>
#include <list>
#include <memory>

#include "cosas/source.h"


// useful for testing

class Constant final : public RelSource {

public:

  Constant(int16_t v); // NOLINT(*-explicit-constructor)
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) override;

private:

  const int16_t value;
};

extern Constant zero;


class Sequence final : public RelSource {

public:

  Sequence(std::initializer_list<int16_t> vs);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) override;

private:

  std::unique_ptr<std::list<int16_t>> values;
};


// this is intended to avoid infinite loops in recursive networks.
// the setter also allows loops to be constructed.

// although this is very obviously mutable it is all declared const
// and implemented with mutable to avoid polluting the Node/Source
// interface.

class Latch final : public RelSource {

public:

  Latch();
  void set_source(RelSource* s);
  int16_t next(int32_t delta, int32_t phi) override;

  friend class SetOnInScope;

private:

  mutable RelSource* source;
  mutable bool on = false;
  mutable int16_t previous = 0;
};


class SetOnInScope {
public:
  explicit SetOnInScope(Latch* l);
  ~SetOnInScope();
private:
  Latch* latch;
};


#endif
