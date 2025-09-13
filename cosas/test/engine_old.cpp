
#include "doctest/doctest.h"

#include "cosas/engine_old.h"


int16_t ff0(RelSource& src, uint32_t n) {
  for (uint32_t i = 0; i < n-1; i++) { src.next(0); }
  return src.next(0);
}


TEST_CASE("Engine, BuildFM_SIMPLE") {
  OldManager m = OldManager();
  CHECK(ff0(m.build(m.FM_SIMPLE), 50) == -91);  // arbitrary values
  CHECK(m.n_panes() == 3);  // carrier, modulator, fm gain/balance
}


TEST_CASE("Engine, BuildFM_LFO") {
  OldManager m = OldManager();
  int32_t amp = ff0(m.build(m.FM_LFO), 123);
  CHECK(amp == 637);  // exact value not important
  CHECK(m.n_panes() == 4);  // carrier, modulator, lfo, fm gain/balance
}


TEST_CASE("Engine, BuildFM_FB") {
  OldManager m = OldManager();
  int32_t amp = ff0(m.build(m.FM_FB), 666);
  CHECK(amp == 2010);  // exact value not important
  CHECK(m.n_panes() == 3);  // carrier/filter, modulator, fm gain/balance/flt balance
}


TEST_CASE("Engine, BuildChord") {
  OldManager m = OldManager();
  m.build(m.CHORD);
}
