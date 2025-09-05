
#ifndef COSAS_RANDOM_H
#define COSAS_RANDOM_H


#include <cstdint>


// https://en.wikipedia.org/wiki/Xorshift

class XorShift32 {
public:
  XorShift32(uint32_t state);
  uint32_t next_uint32();
  int16_t next_int12();
  bool next_bool();
private:
  void next_state();
  uint32_t state;
  uint32_t bit_index;
  static int16_t uint16_to_int12(uint16_t val);
};

#endif

