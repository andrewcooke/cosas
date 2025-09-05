

#include "doctest/doctest.h"

#include "cosas/random.h"


TEST_CASE("XorShift32, uint32") {
  XorShift32 rand = XorShift32(1);
  CHECK(rand.next_uint32() == 270369);
  CHECK(rand.next_uint32() == 67634689);
  CHECK(rand.next_uint32() == 2647435461);
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
