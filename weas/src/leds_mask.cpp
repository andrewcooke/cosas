
#include <algorithm>
#include <math.h>

#include "pico/types.h"
#include "weas/leds_mask.h"


LEDsMask::LEDsMask() : scale() {
   for (uint i = 0; i < N_CYCLE; ++i) scale[i] = 1 + std::sinf(static_cast<float>(2 * M_PI * i / N_CYCLE));
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
    int v1 = static_cast<int>((extra & BITS_MASK) * scale[count & (N_CYCLE - 1)]);
    // leds.set(i, static_cast<uint8_t>(std::min(0xff, std::max(0,v0 + v1))));
    // leds.set(i, static_cast<uint8_t>(std::min(0xff, std::max(0,std::max(v0, v1)))));
    leds.set(i, static_cast<uint8_t>(v0 > v1 ? std::min(0xff, v0 + v1 / 2) : v1));
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
