
#ifndef COSA_PANE_H
#define COSA_PANE_H

#include <memory>

#include "control.h"


// a pane describes what is controlled by the current "view" of the
// ui.  the interface between each ui component and the associated
// software compoonent is managed via a control.  the control does the
// mapping from float 0-1 to whatever is necessary (log, linear,
// range, knob lock etc).

// i guess this should also include output...

class Pane {

  unique_ptr<Input> top_knob;
  unique_ptr<Input> left_knob;
  unique_ptr<Input> right_knob;
  
}


#endif
