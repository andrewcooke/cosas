
#ifndef WEAS_KNOBS_H
#define WEAS_KNOBS_H


#include <cstdint>


class Knob;


class KnobChange {

public:
  enum Highlight {No, Near, Yes};
  KnobChange(Knob *k, float n, Highlight h) : normalized(n), highlight(h), knob(k) {};
  ~KnobChange();
  float normalized;
  Highlight highlight;

private:
  Knob *knob;
};


class Knob {

public:
  friend class KnobChange;
  Knob() = default;
  KnobChange handle_knob_change(uint16_t now, uint16_t prev);

private:
  float normalized = 0.5;
  void apply_change();
};


#endif

