
#ifndef COSA_MATHS_H
#define COSA_MATHS_H

#include <ostream>
#include <cstdint>

#include "cosas/doctest.h"

#include "cosas/constants.h"


inline int16_t clip_16(int64_t val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return static_cast<int16_t>(val);
}

inline int16_t clip_16(int32_t val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return static_cast<int16_t>(val);
}

inline int16_t clip_16(uint32_t val) {
  if (val > sample_max) return sample_max;
  return static_cast<int16_t>(val);
}

inline int16_t clip_16(float val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return static_cast<int16_t>(val);
}

inline uint32_t clip_u32(uint64_t val) {
  if (val > std::numeric_limits<uint32_t>::max()) val = std::numeric_limits<uint32_t>::max();
  return val;
}

inline uint16_t clip_u16(float val) {
  if (val > std::numeric_limits<uint16_t>::max()) val = std::numeric_limits<uint16_t>::max();
  else if (val < 0) val = 0;
  return val;
}

inline uint16_t gcd(uint16_t a, uint16_t b) {
  if (b > a) {uint16_t tmp = a; a = b; b = tmp;}  // a > b
  if (b == 0) return a;
  return gcd(b, a % b);
};


class SimpleRatio {

 public:
  
  SimpleRatio(float val);
  SimpleRatio(int16_t bits, uint8_t scale, bool third, bool fifth);
  uint16_t get_numerator() const;
  uint16_t get_denominator() const;
  uint32_t multiply(uint32_t val) const;
  float as_float() const;
  float error(float other) const;
  bool operator== (const SimpleRatio& other) const;
  
 private:

  friend std::ostream& operator<<(std::ostream& os, const SimpleRatio& sr); 

  int8_t bits;
  uint8_t scale;
  bool third;
  bool fifth;

};


// some scaling factors are converted to 16 bits where the top 8 bits
// are integer and the bottom 8 fractional.  after multiplying the
// result is right shifted 8 bits to drop the fractional part.  this
// is done in 32 bits to avoid clipping intermediate values

// TODO - is this really faster than mult by float?


// for 8 bits decimal, signed

const int one8_bits = 8;
const int16_t one8 = 1 << one8_bits;

inline int16_t scale2mult_shift8(float f) {
  return f * one8;
}

inline int16_t mult_shift8(int16_t k, int16_t x) {
  return (k * static_cast<int32_t>(x)) >> one8_bits;
}

inline int32_t mult_shift8(int16_t k, int32_t x) {
  return (k * static_cast<int64_t>(x)) >> one8_bits;
}

TEST_CASE("MultShift8") {
  CHECK(mult_shift8(scale2mult_shift8(0.33333), INT32_C(300)) == 99);  // almost
  CHECK(mult_shift8(scale2mult_shift8(1), INT32_C(300)) == 300); 
  CHECK(mult_shift8(scale2mult_shift8(-0.33333), INT32_C(300)) == -100);
  CHECK(mult_shift8(scale2mult_shift8(-1), INT32_C(300)) == -300); 
  CHECK(mult_shift8(scale2mult_shift8(0.33333), -INT32_C(300)) == -100);
  CHECK(mult_shift8(scale2mult_shift8(1), -INT32_C(300)) == -300); 
}

// 14 bits decimal, signed

const size_t one14_bits = 14;
const uint16_t one14 = static_cast<uint16_t>(1) << one14_bits;

inline uint16_t scale2mult_shift14(float f) {
  return clip_u16(f * one14);
}

inline int16_t mult_shift14(uint16_t k, int16_t x) {
  bool neg = x < 0;
  int16_t x2 = clip_16((static_cast<uint32_t>(k) * static_cast<uint16_t>(abs(x))) >> one14_bits);
  return neg ? -x2 : x2;
}

TEST_CASE("MultShift14") {
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


class IEEEFloat {

public:

  IEEEFloat(double f);
  IEEEFloat(float f);
  IEEEFloat(int v);
  IEEEFloat(int16_t v);
  IEEEFloat(uint32_t m, uint32_t e, uint32_t s);

  float f();
  uint32_t m();
  uint32_t e();
  uint32_t s();
  int16_t sample();
  void dump(std::ostream& c);
  
private:

  const uint32_t hidden = 1 << 23;
  const uint32_t mask = hidden - 1;

  typedef union {
    float f;
    uint32_t u;
  } float_cast;

  float_cast fc;

};


float sample2float(int16_t s);

int16_t float2sample(float f);


#endif
