
#include <memory>

#include "cosas/engine_small.h"
#include "cosas/debug.h"


RelSource& SmallManager::build(SmallEngine engine) {
  clear_all();
  switch (engine) {
  default:
  case OSCILLATOR:
    return build_oscillator();
  case SIMPLE_2_OSC_FM:
    return build_simple_2_osc_fm();
  }
}

// panes:
//   1 - freq/gain/off
//   2 - freq/shp/asym
RelSource& SmallManager::build_oscillator() {
  return std::get<0>(add_abs_poly_osc_w_gain(440, PolyTable::SINE, 0, QUARTER_TABLE_SIZE, 1.0f));
}

// panes:
//   1 - freq/gain/off
//   2 - freq/shp/asym
//   3 - freq/gain/off
//   4 - freq/shp/asym
RelSource& SmallManager::build_simple_2_osc_fm() {
  auto gc_c = add_abs_poly_osc_w_gain(440, PolyTable::SINE, 0, QUARTER_TABLE_SIZE, 1.0f);
  auto gm_m = add_rel_poly_osc_w_gain(std::get<1>(gc_c).get_freq_param(), PolyTable::SINE, 1, 1, 0.01f);
  // i don't understand this 3.  is it subtick_bits?
  RelSource& fm = add_fm(std::get<0>(gc_c), std::get<0>(gm_m), 1.0f);
  return fm;
}

