
#ifndef COSA_PANE_H
#define COSA_PANE_H

#include <vector>
#include <memory>

#include "control.h"


// a pane describes what is controlled by the current "view" of the
// ui.  the interface between each ui component and the associated
// software compoonent is managed via a control.  the control does the
// mapping from float 0-1 to whatever is necessary (log, linear,
// range, knob lock etc).

// i guess this should also include output...

class Pane {

public:
  
  Pane(std::unique_ptr<Input> top, std::unique_ptr<Input> left, std::unique_ptr<Input> right);

private:
  
  std::unique_ptr<Input> top_knob;
  std::unique_ptr<Input> left_knob;
  std::unique_ptr<Input> right_knob;
  
};


class PaneSet {

public:
  
  PaneSet();
  void append(std::unique_ptr<Pane> p);

private:

  std::vector<std::unique_ptr<Pane>> panes;
  
};
  

#endif
