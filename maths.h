
#ifndef COSA_MATHS_H
#define COSA_MATHS_H

#include <ostream>
#include <cstdint>

#include "doctest.h"

#include "constants.h"


inline int16_t clip_16(int64_t val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return (int16_t)val;
}

inline int16_t clip_16(int32_t val) {
  if (val > sample_max) return sample_max;
  if (val < sample_min) return sample_min;
  return (int16_t)val;
}

inline int16_t clip_16(float val) {
  return clip_16((int32_t)val);
}

inline uint32_t clip_u32(uint64_t val) {
  if (val > std::numeric_limits<uint32_t>::max()) val = std::numeric_limits<uint32_t>::max();
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
  
  uint8_t scale = 1;
  int8_t bits = 0;
  bool third = false;
  bool fifth = false;

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
  return (k * (int32_t)x) >> one8_bits;
}

TEST_CASE("MultShift8") {
  CHECK(mult_shift8(scale2mult_shift8(0.33333), 300) == 99);  // almost
  CHECK(mult_shift8(scale2mult_shift8(1), 300) == 300); 
  CHECK(mult_shift8(scale2mult_shift8(-0.33333), 300) == -100);
  CHECK(mult_shift8(scale2mult_shift8(-1), 300) == -300); 
  CHECK(mult_shift8(scale2mult_shift8(0.33333), -300) == -100);
  CHECK(mult_shift8(scale2mult_shift8(1), -300) == -300); 
}

// 14 bits decimal, signed

const int one14_bits = 14;
const int16_t one14 = 1 << one14_bits;

inline int16_t scale2mult_shift14(float f) {
  return f * one14;
}

inline int16_t mult_shift14(int16_t k, int16_t x) {
  return (k * (int32_t)x) >> one14_bits;
}

TEST_CASE("MultShift14") {
  CHECK(mult_shift14(scale2mult_shift14(0.33333), 300) == 99);  // almost
  CHECK(mult_shift14(scale2mult_shift14(1), 300) == 300);
  CHECK(mult_shift14(scale2mult_shift14(-0.33333), 300) == -100);
  CHECK(mult_shift14(scale2mult_shift14(-1), 300) == -300); 
  CHECK(mult_shift14(scale2mult_shift14(0.33333), -300) == -100);
  CHECK(mult_shift14(scale2mult_shift14(1), -300) == -300); 
}



#endif
