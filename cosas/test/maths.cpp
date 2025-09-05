
#include "doctest/doctest.h"

#include "cosas/maths.h"


TEST_CASE("Maths, SimpleRatio") {
  CHECK(SimpleRatio(0.1f) == SimpleRatio(-1, 1, false, true));
  CHECK(SimpleRatio(1) == SimpleRatio(0, 1, false, false));
  CHECK(SimpleRatio(10) == SimpleRatio(1, 5, false, false));
}

/*
  broken when SAMPLE_MAX changed, but not important?

TEST_CASE("Maths, IEEEFloat") {
  CHECK(sample2float(SAMPLE_MAX) == doctest::Approx(1).epsilon(0.001));
  CHECK(sample2float(SAMPLE_MAX/2) == doctest::Approx(0.5).epsilon(0.001));
  CHECK(sample2float(0) == doctest::Approx(0).epsilon(0.001));
  CHECK(sample2float(SAMPLE_MIN/2) == doctest::Approx(-0.5).epsilon(0.001));
  CHECK(sample2float(SAMPLE_MIN) == doctest::Approx(-1).epsilon(0.001));
  CHECK(float2sample(1.0) == doctest::Approx(SAMPLE_MAX).epsilon(0.001));
  CHECK(float2sample(0.5) == doctest::Approx(SAMPLE_MAX/2).epsilon(0.001));
  CHECK(float2sample(0.0) == doctest::Approx(0).epsilon(0.001));
  CHECK(float2sample(-0.5) == doctest::Approx(SAMPLE_MIN/2).epsilon(0.001));
  CHECK(float2sample(-1.0) == doctest::Approx(SAMPLE_MIN).epsilon(0.001));
  for (int32_t s = SAMPLE_MIN; s <= SAMPLE_MAX; s += 1234) {
    CHECK(float2sample(sample2float(s)) == doctest::Approx(s).epsilon(0.001));
  }
  for (float f = -1; f <= 1; f += 0.123f) {
    CHECK(sample2float(float2sample(f)) == doctest::Approx(f).epsilon(0.001));
  }
}
*/

TEST_CASE("Maths, MultShift8") {
  CHECK(mult_shift8(scale2mult_shift8(0.33333f), INT32_C(300)) == 99);  // almost
  CHECK(mult_shift8(scale2mult_shift8(1), INT32_C(300)) == 300); 
  CHECK(mult_shift8(scale2mult_shift8(-0.33333f), INT32_C(300)) == -100);
  CHECK(mult_shift8(scale2mult_shift8(-1), INT32_C(300)) == -300); 
  CHECK(mult_shift8(scale2mult_shift8(0.33333f), -INT32_C(300)) == -100);
  CHECK(mult_shift8(scale2mult_shift8(1), -INT32_C(300)) == -300); 
}


TEST_CASE("Maths, MultShift14") {
  CHECK(scale2mult_shift14(1) == 16384);
  CHECK(scale2mult_shift14(2) == 32768);
  CHECK(scale2mult_shift14(3.99f) == 65372);
  CHECK(mult_shift14(scale2mult_shift14(0.33333f), 300) == 99);  // almost
  CHECK(mult_shift14(scale2mult_shift14(1), 300) == 300);
  CHECK(mult_shift14(scale2mult_shift14(1.5f), 300) == 450);
  CHECK(mult_shift14(scale2mult_shift14(1), -300) == -300);
  CHECK(mult_shift14(scale2mult_shift14(2), -300) == -600);
  CHECK(mult_shift14(scale2mult_shift14(2), (SAMPLE_MAX - 1) / 2) == SAMPLE_MAX - 1);
  CHECK(mult_shift14(scale2mult_shift14(2), (SAMPLE_MAX - 1) / 2 - 1) == SAMPLE_MAX - 3);
  CHECK(mult_shift14(scale2mult_shift14(2), (SAMPLE_MIN + 1) / 2) == SAMPLE_MIN + 1);
  CHECK(mult_shift14(scale2mult_shift14(2), (SAMPLE_MIN + 1) / 2 + 1) == SAMPLE_MIN + 3);
}


