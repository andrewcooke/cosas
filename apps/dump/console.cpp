
#include <iostream>
#include <cmath>

#include "console.h"
#include "cosas/app_fome.h"


void dump_w_top(OldManager::OldEngine e, size_t n, size_t pane) {
  auto m = OldManager();
  RelSource& fm = m.build(e);
  const auto p = m.get_pane(pane);
  for (size_t i = 0; i < n; i++) {
    p.main.set(i / static_cast<float>(n));
    const int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_w_gain(OldManager::OldEngine e, size_t n) {
  auto m = OldManager();
  RelSource& fm = m.build(e);
  auto p = m.get_pane(m.n_panes()-1);
  for (size_t i = 0; i < n; i++) {
    p.main.set(static_cast<float>(i) / static_cast<float>(n));
    const int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

// void dump_w_wdex(OldManager::OldEngine e, size_t n) {
//   auto m = OldManager();
//   RelSource& fm = m.build(e);
//   auto p = m.get_pane(0);
//   for (size_t i = 0; i < n; i++) {
//     p.x.set(m.n_dex() * i / static_cast<float>(n));
//     const int16_t amp = fm.next(1, 0);
//     std::cout << i << " " << amp << std::endl;
//   }
// }

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

void dump_old(OldManager::OldEngine e, size_t n) {
  auto m = OldManager();
  RelSource& fm = m.build(e);
  for (size_t i = 0; i < n; i++) {
    int16_t amp = fm.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_small(SmallManager::SmallEngine e, size_t n) {
  auto m = SmallManager();
  RelSource& src = m.build(e);
  for (size_t i = 0; i < n; i++) {
    int16_t amp = src.next(1, 0);
    std::cout << i << " " << amp << std::endl;
  }
}

void dump_fome(size_t n) {
  FomeApp app;
  RelSource* source = app.get_source(0);
  app.get_param(0, Main).set(1);
  app.get_param(1, Main).set(-1.0f * QUARTER_TABLE_SIZE);
  app.get_param(1, X).set(PolyTable::SINE+1);
  app.get_param(1, Y).set(5);
  // ParamAdapter knob = ParamAdapter(param);
  // size_t prev_knb = 0;
  for (size_t i = 0; i < n; i++) {
    int16_t amp = source->next(1, 0);
    // size_t next_knob = (i >> 2 & 0xfff);
    // knob.handle_knob_change(next_knob, prev_knb);
    // std::cout << i << " " << amp << " " <<  param.get() << std::endl;
    std::cout << i << " " << amp << std::endl;
    // prev_knb = next_knob;
  }
}