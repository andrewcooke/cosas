
#include <algorithm>
#include <math.h>

#include "pico/types.h"
#include "weas/leds_mask.h"


LEDsMask::LEDsMask() : pulse() {
  // pulse always >= 0
  for (uint i = 0; i < N_CYCLE; ++i) pulse[i] = 1 + std::sinf(static_cast<float>(2 * std::numbers::pi * i / N_CYCLE));
 }


void LEDsMask::show(uint32_t mask) {
  for (uint i = 0; i < leds.N; i++) {
    leds.set(i, static_cast<uint>(mask & BITS_MASK) << (8 - BITS));
    mask = mask >> BITS;
  }
}

void LEDsMask::show(uint32_t mask, uint32_t extra, uint32_t count) {
  for (uint i = 0; i < leds.N; i++) {
    int v0 = static_cast<int>(mask & BITS_MASK) << (8 - BITS);
    int v1 = static_cast<int>((extra & BITS_MASK) * pulse[count & (N_CYCLE - 1)]);
    leds.set(i, static_cast<uint8_t>(std::min(0xff, v0 + v1)));
    mask >>= BITS;
    extra >>= BITS;
  }
}

uint32_t LEDsMask::reverse(uint32_t mask) {
  uint32_t reversed = 0;
  for (uint i = 0; i < LEDs::N; i++) {
    reversed <<= BITS;
    reversed |= mask & BITS_MASK;
    mask = mask >> BITS;
  }
  return reversed;
}

uint32_t LEDsMask::vinterp(uint off, uint32_t a, uint32_t b) {
  switch (off % 4) {
  case 0: return a;
  case 1: return ((a << (2 * BITS)) & FULL_MASK) | ((b & FULL_MASK) >> (4 * BITS));
  case 2: return ((a << (4 * BITS)) & FULL_MASK) | ((b & FULL_MASK) >> (2 * BITS));
  default: return b;
  }
}

uint32_t LEDsMask::hinterp(uint off, uint32_t a, uint32_t b) {
  // TODO - not sure i have this the right way round?  is it l to r?
  switch (off % 3) {
  case 0: return a;
  case 1: return ((a & SIDE_MASK) >> BITS) | ((b << BITS) & SIDE_MASK);
  default: return b;
  }
}

uint32_t LEDsMask::ring(float normalized, bool highlight) {
  uint32_t mask = 0;
  normalized *= LEDs::N;
  mask = overwrite(mask, normalized, 2.5f, 0x07);
  mask = overwrite(mask, normalized, 1.5f, 0x0f);
  if (highlight) mask = overwrite(mask, normalized, 1, 0x1f);
  return mask;
}

uint32_t LEDsMask::overwrite(uint32_t mask, float centre, float width, uint amplitude) {
  float win_lo = centre - 0.5f * width;
  float win_hi = centre + 0.5f * width;
  for (uint led = 0; led < LEDs::N; led++) {
    float led_lo = led;
    float led_hi = led + 1;
    float overlap = 0;
    if (win_lo < led_lo) {
      if (win_hi > led_hi) {
        overlap = 1;
      } else {
        overlap = std::max(0.0f, std::min(1.0f, win_hi - led_lo));
      }
    } else {
      if (win_hi > led_hi) {
        overlap = std::max(0.0f, std::min(1.0f, led_hi - win_lo));
      } else {
        overlap = std::min(1.0f, width);
      }
    }
    uint shift = ring_order[led] * BITS;
    uint new_val = static_cast<uint>(amplitude * overlap) & 0x0f;
    uint old_val = (mask >> shift) & BITS_MASK;
    if (new_val > old_val) {
      mask &= (FULL_MASK - (BITS_MASK << shift));
      mask |= new_val << shift;
    }
  }
  return mask;
}

uint32_t LEDsMask::square(uint bottom, uint amplitude) {
  uint32_t mask = 0;
  for (uint i = 0; i < 4; i++) {
    mask <<= BITS;
    mask |= (amplitude & BITS_MASK);
  }
  if (bottom) mask <<= (2 * BITS);
  return mask;
}

uint32_t LEDsMask::vbar(uint right, uint amplitude) {
  uint32_t mask = 0;
  for (uint i = 0; i < 3; i++) {
    mask <<= (2 * BITS);
    mask |= (amplitude & BITS_MASK);
  }
  if (!right) mask <<= BITS;
  return mask;
}
