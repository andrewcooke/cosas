
#ifndef COSAS_KNOBS_H
#define COSAS_KNOBS_H


class Knob;


// afair the idea here is that this applies the change when it goes out of
// scope (and the destructor is called).  that way the UI updates before the
// (possibly slow) change is made to the system.

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
  Knob(float s, float ln, bool lg, float lo, float hi)
  : scale(s), linearity(ln), log(lg), lo(lo), hi(hi) {};
  Knob() : Knob(1, 1, false, 0, 1) {};
  KnobChange handle_knob_change(uint16_t now, uint16_t prev);

protected:
  // processing on input
  float normalized = 0.5;
  float scale = 1;
  float linearity;  // 0 flat, 1 linear
  // processing on output
  bool log = false;
  float lo = 0;
  float hi = 1;
  void apply_change();
  KnobChange::Highlight ends();
  float clip(float n);
  float sigmoid(uint16_t now, uint16_t prev);
};


#endif

