
#include <cmath>
#include <iostream>

#include "doctest.h"
#include "constants.h"
#include "maths.h"


int16_t clip_16(int64_t val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return (int16_t)val;
}

int16_t clip_16(int32_t val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return (int16_t)val;
}

int16_t clip_16(float val) {
  return clip_16((int32_t)val);
}


uint16_t gcd(uint16_t a, uint16_t b) {
  if (b > a) {uint16_t tmp = a; a = b; b = tmp;}  // a > b
  if (b == 0) return a;
  return gcd(b, a % b);
};


SimpleRatio::SimpleRatio(int16_t b, uint8_t s, bool t, bool f)
  : bits(b), scale(s), third(t), fifth(f) {};

float SimpleRatio::error(float other) const {
  float val = as_float();
  if (val == other) return 0;
  if (val > other) return val / other - 1;
  return other / val - 1;
}

// there's some kind of optimisation possible here, where you do tests
// in order of delta and stop when you pass by the target.  but it's
// non-trivial and only a factor of 2 or so.

// or even a binary search...

SimpleRatio best(float target, SimpleRatio sr1, SimpleRatio sr2) {
  return sr1.error(target) < sr2.error(target) ? sr1 : sr2;
}

// octave is implicit in the base 2 bits so does not need to be
// included below.

SimpleRatio from_below(float target, int8_t bits, SimpleRatio lo) {
  // larger, but no more than double
  SimpleRatio b =
     best(target, lo, SimpleRatio(bits - 1,  3, false, false));  // x 3/2 perfect fifth
  b = best(target, b, SimpleRatio(bits + 2,  1, true,  false));  // x 4/3 perfect fourth
  b = best(target, b, SimpleRatio(bits,      5, true,  false));  // x 5/3 major sixth
  b = best(target, b, SimpleRatio(bits - 2,  5, false, false));  // x 5/4 major third
  b = best(target, b, SimpleRatio(bits + 1,  3, false, true));   // x 6/5 minor third
  b = best(target, b, SimpleRatio(bits + 3,  1, false, true));   // x 8/5 minor sixth

  // extended range
  b = best(target, b, SimpleRatio(bits - 2,  7, false, false));  // x 7/4
  b = best(target, b, SimpleRatio(bits,      7, false, true));   // x 7/5
  b = best(target, b, SimpleRatio(bits,      9, false, true));   // x 9/5
  b = best(target, b, SimpleRatio(bits - 1,  7, true,  false));  // x 7/6
  b = best(target, b, SimpleRatio(bits - 1, 11, true,  false));  // x 11/6

  return b;
}

SimpleRatio from_above(float target, int8_t bits, SimpleRatio hi) {
  // smaller, but no less than half
  SimpleRatio b =
     best(target, hi, SimpleRatio(bits,      3, false, true));   // x 3/5 major sixth
  b = best(target, b, SimpleRatio(bits - 3,  5, false, false));  // x 5/8 minor sixth
  b = best(target, b, SimpleRatio(bits + 1,  1, true,  false));  // x 2/3 perfect fifth
  b = best(target, b, SimpleRatio(bits + 2,  3, false, false));  // x 3/4 perfect fourth
  b = best(target, b, SimpleRatio(bits + 2,  1, false, true));   // x 4/5 major third
  b = best(target, b, SimpleRatio(bits - 1,  5, true,  false));  // x 5/6 minor third

  // matching values for those above are not possible (no seventh, ninth or eleventh)
  b = best(target, b, SimpleRatio(bits - 4,  9, false, false));  // x 9/16
  b = best(target, b, SimpleRatio(bits - 4, 11, false, false));  // x 11/16
  b = best(target, b, SimpleRatio(bits - 4, 13, false, false));  // x 13/16
  b = best(target, b, SimpleRatio(bits - 3,  7, false, false));  // x 7/8
  b = best(target, b, SimpleRatio(bits - 4, 15, false, false));  // x 15/16

  return b;
};

SimpleRatio::SimpleRatio(float target) {
  if (target != 0) {
    int8_t b = log2(target);
    SimpleRatio sr = best(target,
			  from_below(target, b, SimpleRatio(b,     1, false, false)),
			  from_above(target, b, SimpleRatio(b + 1, 1, false, false)));
    bits = sr.bits;
    scale = sr.scale;
    third = sr.third;
    fifth = sr.fifth;
  }
};

uint16_t SimpleRatio::get_numerator() const {
  if (bits > 0) return scale * (1 << bits);
  else return scale;
}

uint16_t SimpleRatio::get_denominator() const {
  uint16_t d = bits < 0 ? 1 << -bits : 1;
  if (third) d = d * 3;
  if (fifth) d = d * 5;
  return d;
}

float SimpleRatio::as_float() const {
  return get_numerator() / (float)get_denominator();
}

uint16_t SimpleRatio::multiply(uint16_t val) const {
  int32_t tmp = val * scale;
  if (third) tmp /= 3;
  if (fifth) tmp /= 5;
  if (bits > 1) tmp <<= bits;
  else tmp >>= -bits;
  return clip_16(tmp);
}

std::ostream& operator<<(std::ostream& os, const SimpleRatio& sr) {
  uint16_t n = sr.get_numerator();
  uint16_t d = sr.get_denominator();
  uint8_t k = gcd(n, d);
  n /= k;
  d /= k;
  if (d == 1) os << n;
  else if (n > d) os << (n / d) << " " << (n % d) << "/" << d;
  else os << n << "/" << d;
  return os;
}

bool SimpleRatio::operator== (const SimpleRatio& other) const {
  return other.bits == bits && other.scale == scale && other.third == third && other.fifth == fifth;
}

TEST_CASE("SimpleRatio") {

  CHECK(SimpleRatio(0.1) == SimpleRatio(-1, 1, false, true));
  CHECK(SimpleRatio(1) == SimpleRatio(0, 1, false, false));
  CHECK(SimpleRatio(10) == SimpleRatio(1, 5, false, false));

}
