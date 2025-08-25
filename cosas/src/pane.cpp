
#include "cosas/pane.h"


Pane::Pane(Param& t, Param& l, Param& r)
  : top(t), left(l), right(r) {};


void PaneSet::append(std::unique_ptr<Pane> p) {
  panes.push_back(std::move(p));
}
