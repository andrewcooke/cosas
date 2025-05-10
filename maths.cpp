
#include "maths.h"


uint16_t gcd(uint16_t a, uint16_t b) {
  if (b > a) {uint16_t tmp = a; a = b; b = tmp;}  // a > b
  if (b == 0) return a;
  return gcd(b, a % b);
}
