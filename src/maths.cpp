
#include <cmath>
#include <iostream>

#include "cosas/maths.h"


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

static SimpleRatio best(float target, SimpleRatio sr1, SimpleRatio sr2) {
  return sr1.error(target) < sr2.error(target) ? sr1 : sr2;
}

// octave is implicit in the base 2 bits so does not need to be
// included below.

static SimpleRatio from_below(float target, int8_t bits, SimpleRatio lo) {
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

static SimpleRatio from_above(float target, int8_t bits, SimpleRatio hi) {
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
  return get_numerator() / static_cast<float>(get_denominator());
}

uint32_t SimpleRatio::multiply(uint32_t val) const {
  uint64_t tmp = val * scale;
  if (third) tmp /= 3;
  if (fifth) tmp /= 5;
  if (bits > 1) tmp <<= bits;
  else tmp >>= -bits;
  return clip_u32(tmp);
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


IEEEFloat::IEEEFloat(double v) : IEEEFloat(static_cast<float>(v)) {};

IEEEFloat::IEEEFloat(float v) : fc({.f=v}) {};

IEEEFloat::IEEEFloat(uint32_t m, uint32_t e, uint32_t s) : fc({.u=0}) {
  fc.u = ((s & 1) << 31) | (e & 255) << 23 | (m & mask);
}

IEEEFloat::IEEEFloat(int v) : IEEEFloat(static_cast<int16_t>(v)) {};

IEEEFloat::IEEEFloat(int16_t v) : fc({.u=0}) {
  if (v != 0) {
    uint32_t s = static_cast<uint32_t>(v < 0) << 31;
    uint32_t e = 127;
    uint32_t m = static_cast<uint32_t>(abs(v)) << 8;
    while (! (m & hidden)) {
      m = m << 1;
      e--;
    }
    fc.u = s | (e << 23) | (m & mask);
  }
}

float IEEEFloat::f() {
  return fc.f;
}

uint32_t IEEEFloat::s() {
  return (fc.u >> 31) & 1;
}

uint32_t IEEEFloat::e() {
  return (fc.u >> 23) & 255;
}

uint32_t IEEEFloat::m() {
  return fc.u & mask;
}

int16_t IEEEFloat::sample() {
  if (e()) {
    int16_t v = static_cast<int16_t>((m() | hidden) >> (8 + 127 - e()));
    if (s()) v = -v;
    return v;
  } else {
    return 0;
  }
}

void IEEEFloat::dump(std::ostream& c) {
  c << fc.f << " (" << std::hex << m() << ", " << std::dec << e() << ", " << s() << ")" << std::endl;
}


float sample2float(int16_t s) {
  return IEEEFloat(s).f();
}

int16_t float2sample(float f) {
  return IEEEFloat(std::max(-0.999969f, std::min(0.999969f, f))).sample();
}

