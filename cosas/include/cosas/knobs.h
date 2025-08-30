
#ifndef COSAS_KNOBS_H
#define COSAS_KNOBS_H

#include "cosas/params.h"


// these are NOT intended for use in cosas code; they're here only because
// i wanted to test them.  see param.h for the cosas "side".


class KnobHandler;


// afair the idea here is that this applies the change when it goes out of
// scope (and the destructor is called).  that way the UI updates before the
// (possibly slow) change is made to the system.

class KnobChange {

public:
  enum Highlight {No, Near, Yes};
  KnobChange(KnobHandler *k, float n, Highlight h) : normalized(n), highlight(h), knob(k) {};
  ~KnobChange();
  float normalized;
  Highlight highlight;

private:
  KnobHandler *knob;
};


class KnobHandler {

public:
  friend class KnobChange;
  KnobHandler(float s, float ln, bool lg, float lo, float hi)
  : scale(s), linearity(ln), log(lg), lo(lo), hi(hi) {};
  KnobHandler() : valid(false) {};
  KnobChange handle_knob_change(uint16_t now, uint16_t prev);
  virtual ~KnobHandler() = default;
  bool is_valid();

protected:
  bool valid = true;
  // processing on input
  float normalized = 0.5;
  float scale = 1;
  float linearity = 1;  // 0 flat, 1 linear
  // processing on output
  bool log = false;
  float lo = 0;
  float hi = 1;
  virtual void apply_change() {};
  KnobChange::Highlight ends();
  float clip(float n);
  float sigmoid(uint16_t now, uint16_t prev);
};


class ParamAdapter final : public KnobHandler {

public:
  explicit ParamAdapter(Param& param);
  void apply_change() override;

private:
  Param& param;

};


#endif

