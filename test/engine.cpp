
#include "doctest/doctest.h"

#include "cosas/engine.h"


TEST_CASE("Engine, BuildFM_SIMPLE") {
  Manager m = Manager();
  CHECK(m.build(m.FM_SIMPLE).next(50, 0) == 12866);
  CHECK(m.n_panes() == 3);  // carrier, modulator, fm gain/balance
}


TEST_CASE("Engine, BuildFM_LFO") {
  Manager m = Manager();
  int32_t amp = m.build(m.FM_LFO).next(123, 0);
  CHECK(amp == 26914);  // exact value not important
  CHECK(m.n_panes() == 4);  // carrier, modulator, lfo, fm gain/balance
}


TEST_CASE("Engine, BuildFM_FB") {
  Manager m = Manager();
  int32_t amp = m.build(m.FM_FB).next(666, 0);
  CHECK(amp == -20828);  // exact value not important
  CHECK(m.n_panes() == 3);  // carrier/filter, modulator, fm gain/balance/flt balance
}


TEST_CASE("Engine, BuildChord") {
  Manager m = Manager();
  m.build(m.CHORD);
}
