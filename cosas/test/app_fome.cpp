
#include "doctest/doctest.h"

#include "cosas/app_fome.h"


TEST_CASE("FomeApp, memory") {
  FomeApp app;
  CHECK(app.n_sources() == 2);
  PhaseSource* src = app.get_source(0);
  CHECK(src->next(1, 0) == 128);
  CHECK(app.n_pages() == 2);
  ParamAdapter k = ParamAdapter(app.get_param(0, Main));
  {
    KnobChange change = k.handle_knob_change(1000, 0);
    // change applied at end of scope
  }
  CHECK(src->next(501, 0) == 1561);
}
