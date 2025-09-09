
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

void dump_fome(uint n, int src) {
  FomeApp app;
  RelSource* source = app.get_source(src);
  // app.get_param(0, Main).set(440);
  // app.get_param(2, Main).set(10);
  app.get_param(2, X).set(10);
  for (size_t i = 0; i < n; i++) {
    int16_t amp = source->next(1, 0);
    int16_t amp2 = app.get_tap(0).prev();
    std::cout << i << " " << amp << " " << amp2 << std::endl;
  }
}