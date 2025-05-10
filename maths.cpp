
#include "maths.h"


uint16_t gcd(uint16_t a, uint16_t b) {
  if (b > a) {uint16_t tmp = a; a = b; b = tmp;}  // a > b
  if (b == 0) return a;
  return gcd(b, a % b);
};


SimpleRatio::SimpleRatio(uint16_t num, uint16_t denom) {
};

SimpleRatio::SimpleRatio(uint8_t b, uint8_t num, bool t, bool f)
  : bits(bits), numerator(num), third(t), fifth(f);

SimpleRatio::as_float() {
  float val = numerator * (1 << bits);
  if (third) val /= 3;
  if (fifth) val /= 5;
  return val;
}

SimpleRatio::error(float other) {
  float val = as_float();
  if (val == other) return 0;
  if (val > other) return val / other - 1;
  return other / val - 1;
}

SimpleRatio::SimpleRatio(float val) {
  SimpleRatio b = best(from_below(SimpleRatio(log2(val),     1, false, false)),
		       from_above(SimpleRatio(log2(val) + 1, 1, false, false)))
  bits = b.bits;
  numerator = b.numerator;
  third = b.third;
  fifth = b.fifth;
};

SimpleRatio best(float target, SimpleRatio a, SimpleRatio b) {
  return a.error(val) < b.error(val) ? a : b;
}

// there's some kind of optimisation possible here, where you do tests
// in order of delta and stop when you pass by the target.  but it's
// non-trivial and only a factor of 2 or so.

// or even a binary search...

SimpleRatio from_below(SimpleRatio lo) {
  // larger, but no more than double
  SimpleRatio b =
     best(lo, SimpleRatio(lo.bits - 1,  3  false, false));  // x 3/2
  b = best(b, SimpleRatio(lo.bits + 2,  1, true,  false));  // x 4/3
  b = best(b, SimpleRatio(lo.bits,      5, true,  false));  // x 5/3
  b = best(b, SimpleRatio(lo.bits - 2,  5, false, false));  // x 5/4
  b = best(b, SimpleRatio(lo.bits - 2,  7, false, false));  // x 7/4
  b = best(b, SimpleRatio(lo.bits + 1,  3, false, true));   // x 6/5
  b = best(b, SimpleRatio(lo.bits,      7, false, true));   // x 7/5
  b = best(b, SimpleRatio(lo.bits + 3,  1, false, true));   // x 8/5
  b = best(b, SimpleRatio(lo.bits,      9, false, true));   // x 9/5
  b = best(b, SimpleRatio(lo.bits - 1,  7, true,  false));  // x 7/6
  b = best(b, SimpleRatio(lo.bits - 1, 11, true,  false));  // x 11/6
  return b;
}

SimpleRatio from_above(SimpleRatio hi) {
  // smaller, but no less that half
  SimpleRatio b =
     best(hi, SimpleRatio(lo.bits + 1,  1  true,  false));  // x 2/3
  b = best(b, SimpleRatio(lo.bits + 2,  1, false, true));   // x 4/5
  b = best(b, SimpleRatio(lo.bits,      3, false, true));   // x 3/5
  b = best(b, SimpleRatio(lo.bits - 1,  5, true,  false));  // x 5/6
  return b;
};

