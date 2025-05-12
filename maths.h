
#ifndef COSA_MATHS_H
#define COSA_MATHS_H

import std;
using namespace std;

#include <cstdint>


uint16_t clip_u16(uint32_t val);
uint16_t clip_u16(int32_t val);
uint16_t clip_u16(float val);

uint8_t clip_u8(int32_t val);
uint8_t clip_u8(float val);


uint16_t gcd(uint16_t a, uint16_t b);


class SimpleRatio {

 public:
  
  SimpleRatio(float val, bool ext);
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
