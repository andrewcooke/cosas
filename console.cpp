
#include <iostream>
#include <cmath>

#include "console.h"


void dump_w_gain(Manager::Engine e, size_t n) {
  auto m = Manager(true);
  const Node& fm = m.build(e);
  auto p = m.get_pane(m.n_panes()-1);
  for (size_t i = 0; i < n; i++) {
    p.top_knob.set(i / static_cast<float>(n) - 0.5);
    int16_t amp = fm.next(i << subtick_bits, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_w_wdex(Manager::Engine e, size_t n) {
  auto m = Manager(true);
  const Node& fm = m.build(e);
  auto p = m.get_pane(0);
  for (size_t i = 0; i < n; i++) {
    p.left_knob.set(m.n_dex() * i / static_cast<float>(n));
    int16_t amp = fm.next(i << subtick_bits, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_poly(float f, size_t shp, size_t asym, size_t off) {
  AbsPolyOsc o = AbsPolyOsc(f, shp, asym, off);
  for (size_t i = 0; i < full_table_size / 440; i++) {
    std::cout << i << " " << o.next(i << subtick_bits, 0) << std::endl;;
  }
}

void dump_dex(float f, Wavelib& w, size_t idx) {
  AbsDexOsc o = AbsDexOsc(w, idx, f);
  for (size_t i = 0; i < full_table_size / 440; i++) {
    std::cout << i << " " << o.next(i << subtick_bits, 0) << std::endl;;
  }
}

void dump(Manager::Engine e, size_t n) {
  auto m = Manager();
  const Node& fm = m.build(e);
  for (size_t i = 0; i < n; i++) {
    int16_t amp = fm.next(i << subtick_bits, 0);
    std::cout << i << " " << amp << std::endl;
  }
}
