
#include <iostream>
#include <cmath>

#include "console.h"


void dump_w_top(Manager::Engine e, size_t n, size_t pane) {
  auto m = Manager(true);
  RelSource& fm = m.build(e);
  const auto p = m.get_pane(pane);
  for (size_t i = 0; i < n; i++) {
    p.top.set(i / static_cast<float>(n));
    const int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_w_gain(Manager::Engine e, size_t n) {
  auto m = Manager(true);
  RelSource& fm = m.build(e);
  auto p = m.get_pane(m.n_panes()-1);
  for (size_t i = 0; i < n; i++) {
    p.top.set(static_cast<float>(i) / static_cast<float>(n));
    const int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_w_wdex(Manager::Engine e, size_t n) {
  auto m = Manager(true);
  RelSource& fm = m.build(e);
  auto p = m.get_pane(0);
  for (size_t i = 0; i < n; i++) {
    p.left.set(m.n_dex() * i / static_cast<float>(n));
    const int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_poly(float f, size_t shp, size_t asym, size_t off, size_t n) {
  AbsPolyOsc o = AbsPolyOsc(f, shp, asym, off);
  for (size_t i = 0; i < n; i++) {
    std::cout << i << " " << o.next(1, 0) << std::endl;;
  }
}

void dump_dex(float f, Wavelib& w, size_t idx) {
  AbsDexOsc o = AbsDexOsc(f, w, idx);
  for (size_t i = 0; i < FULL_TABLE_SIZE / 440; i++) {
    std::cout << i << " " << o.next(1, 0) << std::endl;;
  }
}

void dump(Manager::Engine e, size_t n) {
  auto m = Manager();
  RelSource& fm = m.build(e);
  for (size_t i = 0; i < n; i++) {
    int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}
