
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
  uint16_t multiply(uint16_t val) const;
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

const int one_bits = 8;
const uint32_t one = 1 << one_bits;

inline int32_t scale2mult_shift(float f) {
  return f * one;
}

inline int16_t mult_shift(int32_t k, int16_t x) {
  return clip_16((k * x) >> one_bits);
}

inline int32_t mult_shift(int32_t k, int32_t x) {
  return (k * (int64_t)x) >> one_bits;
}

TEST_CASE("MultShift") {
  CHECK(mult_shift(scale2mult_shift(0.33333), 300) == 99);  // almost
}

#endif
