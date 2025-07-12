
#ifndef COSAS_MATHS_H
#define COSAS_MATHS_H

#include <ostream>
#include <cstdint>

#include "cosas/constants.h"


inline int16_t clip_16(int64_t val) {
  if (val > SAMPLE_MAX) return SAMPLE_MAX;
  if (val < SAMPLE_MIN) return SAMPLE_MIN;
  return static_cast<int16_t>(val);
}

inline int16_t clip_16(int32_t val) {
  if (val > SAMPLE_MAX) return SAMPLE_MAX;
  if (val < SAMPLE_MIN) return SAMPLE_MIN;
  return static_cast<int16_t>(val);
}

inline int16_t clip_16(uint32_t val) {
  if (val > SAMPLE_MAX) return SAMPLE_MAX;
  return static_cast<int16_t>(val);
}

inline int16_t clip_16(float val) {
  if (val > SAMPLE_MAX) return SAMPLE_MAX;
  if (val < SAMPLE_MIN) return SAMPLE_MIN;
  return static_cast<int16_t>(val);
}

inline uint32_t clip_u32(uint64_t val) {
  if (val > std::numeric_limits<uint32_t>::max()) val = std::numeric_limits<uint32_t>::max();
  return val;
}

inline uint16_t clip_u16(float val) {
  if (val > std::numeric_limits<uint16_t>::max()) val = std::numeric_limits<uint16_t>::max();
  else if (val < 0) val = 0;
  return static_cast<uint16_t>(val);
}

inline uint16_t gcd(uint16_t a, uint16_t b) {
  if (b > a) { const uint16_t tmp = a; a = b; b = tmp;}  // a > b
  if (b == 0) return a;
  return gcd(b, a % b);
};


class SimpleRatio {
 public:
  explicit SimpleRatio(float val);
  SimpleRatio(int8_t bits, uint8_t scale, bool t, bool f);
  [[nodiscard]] uint16_t get_numerator() const;
  [[nodiscard]] uint16_t get_denominator() const;
  [[nodiscard]] uint32_t multiply(uint32_t val) const;
  [[nodiscard]] float as_float() const;
  [[nodiscard]] float error(float other) const;
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

constexpr int one8_bits = 8;
constexpr int16_t one8 = 1 << one8_bits;

inline int16_t scale2mult_shift8(float f) {
  return static_cast<int16_t>(f * one8);
}

inline int16_t mult_shift8(int16_t k, int16_t x) {
  return static_cast<int16_t>((k * static_cast<int32_t>(x)) >> one8_bits);
}

inline int32_t mult_shift8(int16_t k, int32_t x) {
  return static_cast<int32_t>((k * static_cast<int64_t>(x)) >> one8_bits);
}

// 14 bits decimal, signed

constexpr size_t one14_bits = 14;
constexpr uint16_t one14 = static_cast<uint16_t>(1) << one14_bits;

inline uint16_t scale2mult_shift14(float f) {
  return clip_u16(f * one14);
}

inline int16_t mult_shift14(uint16_t k, int16_t x) {
  const bool neg = x < 0;
  const int16_t x2 = clip_16((static_cast<uint32_t>(k) * static_cast<uint16_t>(abs(x))) >> one14_bits);
  return neg ? -x2 : x2;
}


class IEEEFloat {
public:
  explicit IEEEFloat(double f);
  explicit IEEEFloat(float f);
  explicit IEEEFloat(int v);
  explicit IEEEFloat(int16_t v);
  IEEEFloat(uint32_t m, uint32_t e, uint32_t s);
  [[nodiscard]] float f() const;
  [[nodiscard]] uint32_t m() const;
  [[nodiscard]] uint32_t e() const;
  [[nodiscard]] uint32_t s() const;
  [[nodiscard]] int16_t sample() const;
  void dump(std::ostream& c) const;
private:
  constexpr static uint32_t HIDDEN = 1 << 23;
  constexpr static uint32_t MASK = HIDDEN - 1;
  typedef union {
    float f;
    uint32_t u;
  } float_cast;
  float_cast fc;
};


float sample2float(int16_t s);

int16_t float2sample(float f);


uint16_t fix_dnl(uint16_t adc);



#endif
