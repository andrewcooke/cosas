
#ifndef COSAS_PANE_H
#define COSAS_PANE_H


#include "cosas/params.h"
#include "cosas/common.h"


// the collection of knobs/params currently on display

class Pane {
public:
  Pane(Param& main, Param& x, Param& y);
  Param& get_param(Knob knob);
  Param& main;
  Param& x;
  Param& y;
};


#endif
