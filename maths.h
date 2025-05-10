
#ifndef FMCOSA_MATHS_H
#define FMCOSA_MATHS_H

import std;
using namespace std;

#include <cstdint>


uint16_t clip_u16(uint32_t inter);
uint16_t clip_u16(int32_t inter);
uint16_t clip_u16(float inter);


uint16_t gcd(uint16_t a, uint16_t b);


class SimpleRatio {

 public:
  
  SimpleRatio(uint16_t num, uint16_t denom);
  SimpleRatio(float val);
  SimpleRatio(int16_t bits, uint8_t scale, bool third, bool fifth);
  uint16_t get_numerator() const;
  uint16_t get_denominator() const;
  uint16_t multiply(uint16_t val) const;
  float as_float() const;
  float error(float other) const;
  bool operator== (const SimpleRatio& other) const;
  
 private:

  friend ostream& operator<<(ostream& os, const SimpleRatio& sr); 
  
  uint8_t scale = 1;
  int16_t bits = 0;
  bool third = false;
  bool fifth = false;

};

#endif
