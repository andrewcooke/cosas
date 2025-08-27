
#include <memory>

#include "cosas/engine_small.h"


RelSource& SmallManager::build(SmallEngine engine) {
  current_sources->clear();
  current_params->clear();
  current_panes->clear();

  switch (engine) {
  default:
  case POLY:
    return build_poly();
  }
}

// panes:
//   1 - freq/blk/blk
//   2 - off/shp/asym
RelSource& SmallManager::build_poly() {
  auto [f, o] = add_abs_poly_osc(10, PolyTable::LINEAR - 1, 0,
                                                   static_cast<size_t>(0.1f * QUARTER_TABLE_SIZE));
  add_pane(f, add_param<Blank>(), add_param<Blank>());
  swap_panes(0, 1);
  return o;
}

