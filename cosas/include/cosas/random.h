
#ifndef COSAS_RANDOM_H
#define COSAS_RANDOM_H


#include <cstdint>


// https://en.wikipedia.org/wiki/Xorshift

class XorShift32 {
public:
  XorShift32(uint32_t state);
  uint32_t next_uint32();
  bool next_bool();
private:
  uint32_t state;
  uint32_t bit_index;
};

#endif

