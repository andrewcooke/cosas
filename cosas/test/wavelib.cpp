
#include "doctest/doctest.h"

#include "cosas/wavelib.h"


// TODO - are these subticks still ok?
TEST_CASE("Wavelib, Square") {
  Wavelib w = Wavelib();
  CHECK(w[w.square_duty_05].next(0 << SUBTICK_BITS) == SAMPLE_MAX);
  CHECK(w[w.square_duty_05].next(HALF_TABLE_SIZE << SUBTICK_BITS) == SAMPLE_MAX);
  CHECK(w[w.square_duty_05].next((HALF_TABLE_SIZE+1) << SUBTICK_BITS) == SAMPLE_MIN);
  CHECK(w[w.square_duty_05].next((FULL_TABLE_SIZE-1) << SUBTICK_BITS) == SAMPLE_MIN);
  CHECK(w[w.square_duty_05].next(FULL_TABLE_SIZE << SUBTICK_BITS) == SAMPLE_MAX);
}
