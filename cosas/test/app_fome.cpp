
#include "doctest/doctest.h"

#include "cosas/app_fome.h"


TEST_CASE("FomeApp, memory") {
  FomeApp app;
  CHECK(app.n_sources() == 7);
  RelSource& src = app.get_source(0);
  CHECK(src.next(1, 0) == 2052);
  CHECK(app.n_pages() == 1);
  KnobHandler& k = app.get_knob(0, Main);
  {
    KnobChange change = k.handle_knob_change(1000, 0);
    // change applied at end of scope
  }
  CHECK(src.next(1, 0) == 4097);
}
