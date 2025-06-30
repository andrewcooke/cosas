
#include "doctest/doctest.h"

#include "cosas/wavetable.h"


TEST_CASE("Wavetable, PolyTable") {
  PolyTable p1 = PolyTable(PolyTable::SQUARE, 0, QUARTER_TABLE_SIZE);
  for (size_t i = 0; i < QUARTER_TABLE_SIZE; i++) {
    CHECK(p1.next(i << SUBTICK_BITS, 0) == SAMPLE_MIN);
    CHECK(p1.next((i + QUARTER_TABLE_SIZE) << SUBTICK_BITS, 0) == SAMPLE_MAX);
  }
  PolyTable p2 = PolyTable(PolyTable::SINE, 0, QUARTER_TABLE_SIZE);
  CHECK(p2.next(0, 0) == 0);
  CHECK(p2.next(QUARTER_TABLE_SIZE << SUBTICK_BITS, 0) == SAMPLE_MAX);
  CHECK(p2.next(HALF_TABLE_SIZE << SUBTICK_BITS, 0) == -4);  // almost 0
  CHECK(p2.next((HALF_TABLE_SIZE + QUARTER_TABLE_SIZE) << SUBTICK_BITS, 0) == SAMPLE_MIN + 1);  // almost
}
