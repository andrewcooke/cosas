
#include "pane.h"


Pane::Pane(std::unique_ptr<Input> top, std::unique_ptr<Input> left, std::unique_ptr<Input> right)
  : top_knob(std::move(top)), left_knob(std::move(left)), right_knob(std::move(right)) {};


PaneSet::PaneSet() : panes() {};

void PaneSet::append(std::unique_ptr<Pane> p) {
  panes.push_back(std::move(p));
}
