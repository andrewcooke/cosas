
#include <iostream>
#include <cmath>

#include "console.h"


void dump_w_gain(Manager::Engine e, size_t n) {
  auto m = Manager();
  const Node& fm = m.build(e);
  auto p = m.get_pane(m.n_panes()-1);
  for (size_t i = 0; i < n; i++) {
    p.top_knob.set(i / static_cast<float>(n) - 0.5);
    int16_t amp = fm.next(i, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_w_wdex(Manager::Engine e, size_t n) {
  auto m = Manager();
  const Node& fm = m.build(e);
  auto p = m.get_pane(0);
  for (size_t i = 0; i < n; i++) {
    p.left_knob.set(i / static_cast<float>(n) - 0.5);
    int16_t amp = fm.next(i, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump(Manager::Engine e, size_t n) {
  auto m = Manager();
  const Node& fm = m.build(e);
  for (size_t i = 0; i < n; i++) {
    int16_t amp = fm.next(i, 0);
    std::cout << i << " " << amp << std::endl;
  }
}
