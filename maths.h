
#ifndef FMCOSA_MATHS_H
#define FMCOSA_MATHS_H

#include <cstdint>


uint16_t gcd(uint16_t a, uint16_t b);


class SimpleRatio {

 public:
  
  SimpleRatio(uint16_t num, uint16_t denom);
  SimpleRatio(float val);
  uint16_t multiply(uint16_t val);
  float as_float();
  
 private:

  SimpleRatio(uint8_t bits, uint8_t numerator, bool third, bool fifth);
  float error(val);
  
  uint8_t bits;
  uint8_t numerator;
  bool third;
  bool fifth;

};

#endif
