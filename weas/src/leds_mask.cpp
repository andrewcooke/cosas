
#include "weas/leds_mask.h"

#include "weas/leds.h"


void LEDsMask::show(uint32_t mask) {
  auto& leds = LEDs::get();
  for (uint i = 0; i < N; i++) {
    leds.set(i, static_cast<uint8_t>((mask & BITS_MASK) << (8 - BITS)));
    mask >>= BITS;
  }
}
