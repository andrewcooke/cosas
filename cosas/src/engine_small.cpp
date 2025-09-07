
#include <memory>

#include "cosas/engine_small.h"


RelSource& SmallManager::build(SmallEngine engine) {
  current_sources->clear();
  current_params->clear();
  current_panes->clear();

  switch (engine) {
  default:
  case OSCILLATOR:
    return build_oscillator();
  case SIMPLE_2_OSC_FM:
    return build_simple_2_osc_fm();
  }
}

// panes:
//   1 - freq/gain/blk
//   2 - off/shp/asym
RelSource& SmallManager::build_oscillator() {
  auto [f, o] = add_abs_poly_osc(10, PolyTable::LINEAR, 0, QUARTER_TABLE_SIZE);
  Gain& g = add_source<Gain>(o, 1.0f, false);
  add_pane(f, g.get_amp(), add_param<Blank>());
  swap_panes(0, 1);
  return g;
}

// panes:
//   1 - freq/gain/blk
//   2 - off/shp/asym
//   3 - freq/gain/blk
//   4 - off/shp/asym
RelSource& SmallManager::build_simple_2_osc_fm() {
  // auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  // RelSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  // // i don't understand this 3.  is it subtick_bits?
  // RelSource& fm = add_fm(c, m, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 3)));
  // return fm;
  return build_oscillator();
}

