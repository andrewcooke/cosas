
#include <memory>

#include "cosas/engine.h"
#include "cosas/constants.h"

#include "console.h"


int main() {
  //  dump_w_gain(Manager::Engine::FM_FB, 0.1 * sample_rate);
  //  dump(Manager::Engine::CHORD, 0.1 * sample_rate);};
  //  dump_w_gain(Manager::Engine::FM_SIMPLE, 0.1 * sample_rate);
  //  dump_w_wdex(Manager::Engine::FM_SIMPLE, 0.1 * sample_rate);
  //  dump(Manager::Engine::DEX, 0.3 * sample_rate);
  //  dump(Manager::Engine::POLY, 1 * sample_rate);
  //  for (size_t asym = 0; asym <= PolyTable::n_shapes; ++asym) {
  //    dump_poly(440, PolyTable::sine, asym, quarter_table_size/2);
  //  }
  //  dump_poly(440, PolyTable::square, 0, quarter_table_size);
  //  dump_poly(10, PolyTable::linear - 1, PolyTable::n_shapes, 0.1 * quarter_table_size, 0.1 * full_table_size);
  //  Wavelib w = Wavelib();
  //  dump_dex(440, w, w.sine_gamma_1);
  //  dump(Manager::Engine::FM_SIMPLE, 0.1 * sample_rate);
  //  dump_w_gain(Manager::Engine::FM_ENV, 0.3 * sample_rate);
  dump_w_top(Manager::Engine::CHORD, 0.3 * SAMPLE_RATE, 1); // weight of first overtone
}
