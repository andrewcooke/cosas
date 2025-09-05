

#include "doctest/doctest.h"

#include "cosas/random.h"


TEST_CASE("XorShift32, uint32") {
  XorShift32 rand = XorShift32(1);
  CHECK(rand.next_uint32() == 270369);
  CHECK(rand.next_uint32() == 67634689);
  CHECK(rand.next_uint32() == 2647435461);
}

TEST_CASE("XorShift32, int12") {
  XorShift32 rand = XorShift32(1);
  CHECK(rand.next_int12() == -2015);
  CHECK(rand.next_int12() == -2044);
  CHECK(rand.next_int12() == -511);
  CHECK(rand.next_int12() == -1016);
  CHECK(rand.next_int12() == 197);
  CHECK(rand.next_int12() == 1484);
}

TEST_CASE("XorShift32, bool") {
  XorShift32 rand = XorShift32(1);
  CHECK(rand.next_bool());
  CHECK(!rand.next_bool());
  CHECK(!rand.next_bool());
  for (uint32_t i = 0; i < 32-4; ++i) rand.next_bool();
  CHECK(!rand.next_bool());
  CHECK(rand.next_bool());
}
