
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
  Knob(float s) : scale(s) {};
  Knob() : Knob(1) {};
  virtual KnobChange handle_knob_change(uint16_t now, uint16_t prev);

protected:
  float normalized = 0.5;
  float scale = 1;
  void apply_change();
  KnobChange::Highlight ends();
  float clip(float n);
};


class Sigmoid : public Knob {
public:
  Sigmoid(float s, float l) : Knob(s), linearity(l) {};
  KnobChange handle_knob_change(uint16_t now, uint16_t prev) override;
private:
  float linearity;  // 0 flat, 1 linear
};


#endif

