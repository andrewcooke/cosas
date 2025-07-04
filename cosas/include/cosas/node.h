
#ifndef COSA_NODE_H
#define COSA_NODE_H

#include <initializer_list>
#include <list>
#include <memory>

#include "cosas/source.h"


// useful for testing

class Constant final : public RelSource {

public:

  Constant(int16_t v); // NOLINT(*-explicit-constructor)
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;

private:

  const int16_t value;
};

extern Constant zero;


class Sequence final : public RelSource {

public:

  Sequence(std::initializer_list<int16_t> vs);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;

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
  void set_source(const RelSource* s) const;
  int16_t next(int32_t delta, int32_t phi) const override;

  friend class SetOnInScope;

private:

  mutable const RelSource* source;
  mutable bool on = false;
  mutable int16_t previous = 0;
};


class SetOnInScope {
public:
  explicit SetOnInScope(const Latch* l);
  ~SetOnInScope();
private:
  const Latch* latch;
};


#endif
