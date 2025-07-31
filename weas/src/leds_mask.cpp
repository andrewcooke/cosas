
#include "weas/leds_mask.h"

#include <algorithm>

#include "pico/types.h"


void LEDsMask::show(uint32_t mask) {
  for (uint i = 0; i < leds.N; i++) {
    leds.set(i, static_cast<uint>(mask & BITS_MASK) << (8 - BITS));
    mask = mask >> BITS;
  }
}

uint32_t LEDsMask::modulate(uint32_t mask, uint32_t extra, uint32_t count) {
  if (!extra) return mask;
  float k = (4 - (count & 0x7)) / 4.0f;
  uint32_t sum = 0;
  for (uint i = 0; i < LEDs::N; i++) {
    sum <<= BITS;
    uint m = mask & BITS_MASK;
    uint e = extra & BITS_MASK;
    uint s = std::max(0u, std::min(BITS_MASK, static_cast<uint>(m + e * k)));
    sum |= s & BITS_MASK;
    mask >>= BITS;
    extra >>= BITS;
  }
  return reverse(sum);
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

uint32_t LEDsMask::top_square(uint amplitude) {
  uint32_t mask = 0;
  for (uint i = 0; i < 4; i++) {
    mask <<= BITS;
    mask |= (amplitude & BITS_MASK);
  }
  return mask;
}
