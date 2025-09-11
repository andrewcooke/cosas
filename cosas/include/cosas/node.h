
#ifndef COSAS_NODE_H
#define COSAS_NODE_H

#include <initializer_list>
#include <list>
#include <memory>

#include "cosas/source.h"


// useful for testing

class Constant final : public PhaseSource {

public:

  explicit Constant(int16_t v);
  [[nodiscard]] int16_t next(uint32_t tick, int32_t phi) override;

private:

  const int16_t value;
};

extern Constant zero;


class Sequence final : public PhaseSource {

public:

  Sequence(std::initializer_list<int16_t> vs);
  [[nodiscard]] int16_t next(uint32_t tick, int32_t phi) override;

private:

  std::unique_ptr<std::list<int16_t>> values;
};


// this is intended to avoid infinite loops in recursive networks.
// the setter also allows loops to be constructed.

// although this is very obviously mutable it is all declared const
// and implemented with mutable to avoid polluting the Node/Source
// interface.

class Latch final : public PhaseSource {

public:

  Latch();
  void set_source(PhaseSource* s);
  int16_t next(uint32_t tick, int32_t phi) override;

  friend class SetOnInScope;

private:

  mutable PhaseSource* source;
  mutable bool on = false;
  mutable int16_t previous = 0;
};

// TODO _ define inside Latch
class SetOnInScope {
public:
  explicit SetOnInScope(Latch* l);
  ~SetOnInScope();
private:
  Latch* latch;
};


class TapMixin {
public:
  TapMixin() = default;
  virtual ~TapMixin() = default;
  virtual int16_t prev() {return previous;};
protected:
  int16_t previous = 0;
};


class OptionalTap : public TapMixin {
public:
  OptionalTap() {};
  ~OptionalTap() override = default;
  void set(TapMixin* t) {tap = t;}
  int16_t prev() override {
    if (tap) return tap->prev();
    return previous;
  };
private:
  TapMixin* tap = nullptr;
};


#endif
