
#include "CppUTest/TestHarness.h"

#include "cosas/wavelib.h"


TEST_GROUP(Wavelib) {}

TEST(Wavelib, Square) {
  Wavelib w = Wavelib();
  CHECK(w[w.square_duty_05].next(0 << subtick_bits) == sample_max);
  CHECK(w[w.square_duty_05].next(half_table_size << subtick_bits) == sample_max);
  CHECK(w[w.square_duty_05].next((half_table_size+1) << subtick_bits) == sample_min);
  CHECK(w[w.square_duty_05].next((full_table_size-1) << subtick_bits) == sample_min);
  CHECK(w[w.square_duty_05].next(full_table_size << subtick_bits) == sample_max);
}
