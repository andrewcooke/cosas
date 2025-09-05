
#include "cosas/random.h"

#include <ctime>


XorShift32::XorShift32(uint32_t state) : state(state), bit_index(0) {
  // autocomplete suggested this and it's not a bad idea (but is time available on pico?)
  if (!state) state = static_cast<uint32_t>(std::time(0));
};

uint32_t XorShift32::next_uint32() {
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  bit_index = 0;
  return state = x;
}

bool XorShift32::next_bool() {
  if (bit_index == 32) next_uint32();
  return state >> bit_index++ & 1;
}
