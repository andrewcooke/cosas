
#include "doctest/doctest.h"

#include "cosas/engine_old.h"

TEST_CASE("Engine, BuildFM_SIMPLE") {
  OldManager m = OldManager();
  CHECK(m.build(m.FM_SIMPLE).next(50, 0) == -7);  // arbitrary values
  CHECK(m.n_panes() == 3);  // carrier, modulator, fm gain/balance
}


TEST_CASE("Engine, BuildFM_LFO") {
  OldManager m = OldManager();
  int32_t amp = m.build(m.FM_LFO).next(123, 0);
  CHECK(amp == 1301);  // exact value not important
  CHECK(m.n_panes() == 4);  // carrier, modulator, lfo, fm gain/balance
}


TEST_CASE("Engine, BuildFM_FB") {
  OldManager m = OldManager();
  int32_t amp = m.build(m.FM_FB).next(666, 0);
  CHECK(amp == 1959);  // exact value not important
  CHECK(m.n_panes() == 3);  // carrier/filter, modulator, fm gain/balance/flt balance
}


TEST_CASE("Engine, BuildChord") {
  OldManager m = OldManager();
  m.build(m.CHORD);
}
