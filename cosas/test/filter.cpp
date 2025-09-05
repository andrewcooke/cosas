
#include <cstddef>
#include <iostream>

#include "doctest/doctest.h"

#include "cosas/filter.h"

#include <newlib/c++/14.2.1/cmath>

TEST_CASE("MovingAverage, next") {
  MovingAverage<uint16_t, 2> f;
  CHECK(f.next(2) == 0);
  for (size_t i = 1; i < 3; i++) CHECK(f.next(2) == 1);
  CHECK(f.next(2) == 2);
}

TEST_CASE("MovingAverage, next_or_2") {
  MovingAverage<uint16_t, 2> f;
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 2);
  CHECK(f.next_or(2, 1, 4096) == 4096);
}

TEST_CASE("MovingAverage, next_or_3") {
  MovingAverage<uint16_t, 3> f;
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 4096);
  CHECK(f.next_or(2, 1, 4096) == 2);
  CHECK(f.next_or(2, 1, 4096) == 4096);
}


TEST_CASE("ThresholdRange") {
  ThresholdRange tr = ThresholdRange(2);
  CHECK(!tr.add(10, 10));
  CHECK(!tr.add(12, 10));
  CHECK(tr.add(13, 12));
  CHECK(tr.now == 13);
  CHECK(tr.prev == 10);
}