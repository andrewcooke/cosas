
#include "doctest/doctest.h"

#include "cosas/wavetable.h"


TEST_CASE("Wavetable, PolyTable") {
  PolyTable p1 = PolyTable(PolyTable::square, 0, quarter_table_size);
  for (size_t i = 0; i < quarter_table_size; i++) {
    CHECK(p1.next(i << subtick_bits, 0) == sample_min);
    CHECK(p1.next((i + quarter_table_size) << subtick_bits, 0) == sample_max);
  }
  PolyTable p2 = PolyTable(PolyTable::sine, 0, quarter_table_size);
  CHECK(p2.next(0, 0) == 0);
  CHECK(p2.next(quarter_table_size << subtick_bits, 0) == sample_max);
  CHECK(p2.next(half_table_size << subtick_bits, 0) == -4);  // almost 0
  CHECK(p2.next((half_table_size + quarter_table_size) << subtick_bits, 0) == sample_min + 1);  // almost
}
