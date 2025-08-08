
#include <cstddef>
#include <iostream>

#include "doctest/doctest.h"

#include "cosas/filter.h"

#include <newlib/c++/14.2.1/cmath>

TEST_CASE("Filter, SelfModLP") {
  SelfModLP f(12, 48000, 600, 0.01f);
  CHECK(f.next(static_cast<uint16_t>(1024)) == 5);
  for (size_t i = 0; i < 4096; i++) f.next(static_cast<uint16_t>(i));
  CHECK(f.next(static_cast<uint16_t>(4095)) == 4086);
}

TEST_CASE("Filter, SelfModLP_signed") {
  SelfModLP f(12, 100, 5, 0.01f);
  for (size_t i = 0; i < 100; i++) /* std::cout << */ f.next(static_cast<int16_t>(2047 * (sin(2 * i * std::numbers::pi) + sin(2 * i * std::numbers::pi / 100)))) /* << std::endl */;
  CHECK(f.next(static_cast<int16_t>(sin(2 * 101 * std::numbers::pi / 100))) == -119);
  CHECK(f.next(static_cast<int16_t>(sin(2 * 102 * std::numbers::pi / 100))) == 0);
}

TEST_CASE("Filter, SelfModLP_or") {
  SelfModLP f(12, 10, 1, 0.5f);
  CHECK(f.next_or(static_cast<uint16_t>(0), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1024), 4096) == 246);
  CHECK(f.next_or(static_cast<uint16_t>(1024), 4096) == 1024);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
  CHECK(f.next_or(static_cast<uint16_t>(1026), 4096) == 4096);
}


TEST_CASE("MovingAverage, next") {
  MovingAverage<2> f(4096);
  CHECK(f.next(2, 0) == 0);
  for (size_t i = 1; i < 3; i++) CHECK(f.next(2, i) == 1);
  CHECK(f.next(2, 3) == 2);
}

TEST_CASE("MovingAverage, next_or") {
  MovingAverage<2> f(4096);
  CHECK(f.next_or(2, 0) == 0);
  CHECK(f.next_or(2, 1) == 1);
  CHECK(f.next_or(2, 2) == 4096);
  CHECK(f.next_or(2, 3) == 2);
}
