
#include "CppUTest/TestHarness.h"

#include "cosas/maths.h"

TEST_GROUP(Maths) {};


TEST(Maths, SimpleRatio) {

  CHECK(SimpleRatio(0.1) == SimpleRatio(-1, 1, false, true));
  CHECK(SimpleRatio(1) == SimpleRatio(0, 1, false, false));
  CHECK(SimpleRatio(10) == SimpleRatio(1, 5, false, false));

}


TEST(Maths, IEEEFloat) {
  CHECK(sample2float(sample_max) == doctest::Approx(1).epsilon(0.001));
  CHECK(sample2float(sample_max/2) == doctest::Approx(0.5).epsilon(0.001));
  CHECK(sample2float(0) == doctest::Approx(0).epsilon(0.001));
  CHECK(sample2float(sample_min/2) == doctest::Approx(-0.5).epsilon(0.001));
  CHECK(sample2float(sample_min) == doctest::Approx(-1).epsilon(0.001));
  CHECK(float2sample(1.0) == doctest::Approx(sample_max).epsilon(0.001));
  CHECK(float2sample(0.5) == doctest::Approx(sample_max/2).epsilon(0.001));
  CHECK(float2sample(0.0) == doctest::Approx(0).epsilon(0.001));
  CHECK(float2sample(-0.5) == doctest::Approx(sample_min/2).epsilon(0.001));
  CHECK(float2sample(-1.0) == doctest::Approx(sample_min).epsilon(0.001));
  for (int32_t s = sample_min; s <= sample_max; s += 1234) {
    CHECK(float2sample(sample2float(s)) == doctest::Approx(s).epsilon(0.001));
  }
  for (float f = -1; f <= 1; f += 0.123) {
    CHECK(sample2float(float2sample(f)) == doctest::Approx(f).epsilon(0.001));
  }
}


TEST(Maths, MultShift8) {
  CHECK(mult_shift8(scale2mult_shift8(0.33333), INT32_C(300)) == 99);  // almost
  CHECK(mult_shift8(scale2mult_shift8(1), INT32_C(300)) == 300); 
  CHECK(mult_shift8(scale2mult_shift8(-0.33333), INT32_C(300)) == -100);
  CHECK(mult_shift8(scale2mult_shift8(-1), INT32_C(300)) == -300); 
  CHECK(mult_shift8(scale2mult_shift8(0.33333), -INT32_C(300)) == -100);
  CHECK(mult_shift8(scale2mult_shift8(1), -INT32_C(300)) == -300); 
}


TEST(Maths, MultShift14) {
  CHECK(scale2mult_shift14(1) == 16384);
  CHECK(scale2mult_shift14(2) == 32768);
  CHECK(scale2mult_shift14(3.99) == 65372);
  CHECK(mult_shift14(scale2mult_shift14(0.33333), 300) == 99);  // almost
  CHECK(mult_shift14(scale2mult_shift14(1), 300) == 300);
  CHECK(mult_shift14(scale2mult_shift14(1.5), 300) == 450);
  CHECK(mult_shift14(scale2mult_shift14(1), -300) == -300);
  CHECK(mult_shift14(scale2mult_shift14(2), -300) == -600);
  CHECK(mult_shift14(scale2mult_shift14(2), (sample_max - 1) / 2) == sample_max - 1); 
  CHECK(mult_shift14(scale2mult_shift14(2), (sample_max - 1) / 2 - 1) == sample_max - 3); 
  CHECK(mult_shift14(scale2mult_shift14(2), (sample_min + 1) / 2) == sample_min + 1); 
  CHECK(mult_shift14(scale2mult_shift14(2), (sample_min + 1) / 2 + 1) == sample_min + 3); 
}


