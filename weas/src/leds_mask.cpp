
#include <algorithm>
#include <math.h>

#include "pico/types.h"
#include "weas/leds_mask.h"


void LEDsMask::show(LEDs& leds, uint32_t mask) {
  for (uint i = 0; i < leds.N; i++) {
    leds.set(i, static_cast<uint>(mask & BITS_MASK) << (8 - BITS));
    mask = mask >> BITS;
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
  if (right) mask <<= BITS;
  return mask;
}

uint32_t LEDsMask::rot2dot(uint off, uint amplitude, bool inverse) {
  uint32_t dot = amplitude & BITS_MASK;
  uint32_t hbase = dot << BITS | dot;
  uint32_t vbase = dot << (2 * BITS) | dot;
  uint32_t mask = 0;
  switch(off % 6) {
  default:
  case 0: mask = hbase << (4 * BITS); break;
  case 1: mask = vbase << (2 * BITS); break;
  case 2: mask = vbase; break;
  case 3: mask = hbase; break;
  case 4: mask = vbase << BITS; break;
  case 5: mask = vbase << (3 * BITS); break;
  }
  if (inverse) mask = mask ^ FULL_MASK;
  return mask;
}
