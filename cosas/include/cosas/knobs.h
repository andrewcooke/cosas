
#ifndef COSAS_KNOBS_H
#define COSAS_KNOBS_H


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
  Knob(float s, float ln, bool lg, float lo, float hi)
  : scale(s), linearity(ln), log(lg), lo(lo), hi(hi) {};
  Knob() : Knob(1, 1, false, 0, 1) {};
  virtual KnobChange handle_knob_change(uint16_t now, uint16_t prev);

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
  float absolute(uint16_t now, uint16_t prev);
  float linear(uint16_t now, uint16_t prev);
  float sigmoid(uint16_t now, uint16_t prev);
};


#endif

