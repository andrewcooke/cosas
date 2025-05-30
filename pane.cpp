
#include "pane.h"


Pane::Pane(Input& top, Input& left, Input& right)
  : top_knob(top), left_knob(left), right_knob(right) {};


PaneSet::PaneSet() : panes() {};

void PaneSet::append(std::unique_ptr<Pane> p) {
  panes.push_back(std::move(p));
}
