
#include "cosas/pane.h"


Pane::Pane(Param& main, Param& x, Param& y)
  : main(main), x(x), y(y) {};

Param &Pane::get_param(Knob knob) {
  switch (knob) {
  default:
  case Main: return main;
  case X: return x;
  case Y: return y;
  }
}
