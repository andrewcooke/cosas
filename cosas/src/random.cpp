
#include "cosas/random.h"

#include "cosas/wavetable.h"

#include <ctime>


XorShift32::XorShift32(uint32_t s) : state(s), bit_index(0) {
  // autocomplete suggested this and it's not a bad idea (but is time available on pico?)
  if (!state) state = static_cast<uint32_t>(std::time(0));
  next_state();
};

void XorShift32::next_state() {
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;
  bit_index = 0;
}

uint32_t XorShift32::next_uint32() {
  if (bit_index) next_state();
  bit_index = 32;
  return state;
}

int16_t XorShift32::uint16_to_int12(uint16_t val) {
  val &= 0xfff;
  int16_t sign = static_cast<int16_t>(val) - 0x800;
  if (sign < SAMPLE_MIN) sign = 0;  // we don't use complete range
  return sign;
}

int16_t XorShift32::next_int12() {
  while (true) {
    if (bit_index == 0) {
      int16_t sign = uint16_to_int12(static_cast<uint16_t>(state & 0xffff));
      bit_index = 16;
      return sign;
    }
    if (bit_index <= 16) {
      int16_t sign = uint16_to_int12(static_cast<uint16_t>(state >> 16));
      bit_index = 32;
      return sign;
    }
    next_state();
  }
}

bool XorShift32::next_bool() {
  if (bit_index == 32) next_state();
  return state >> bit_index++ & 1;
}
