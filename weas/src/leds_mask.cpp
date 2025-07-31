
#include "weas/leds_mask.h"

#include <algorithm>

#include "pico/types.h"


void LEDsMask::show(uint32_t s) {
  for (uint i = 0; i < leds.N; i++) {
    leds.set(i, static_cast<uint>(s & 0x0fu) << 4);
    s = s >> 4;
  }
}


uint32_t LEDsMask::ring(float normalized, bool highlight) {
  uint32_t mask = 0;
  normalized *= LEDs::N;
  mask = overwrite(mask, normalized, 2.5f, 0x03);
  mask = overwrite(mask, normalized, 1.5f, 0x07);
  if (highlight) mask = overwrite(mask, normalized, 1, 0x0f);
  return mask;
}

uint32_t LEDsMask::overwrite(uint32_t mask, float n, float w, uint a) {
  float win_lo = n - 0.5f * w;
  float win_hi = n + 0.5f * w;
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
        overlap = std::min(1.0f, w);
      }
    }
    uint shift = ring_order[led] * 4;
    uint new_val = static_cast<uint>(a * overlap) & 0x0f;
    uint old_val = (mask >> shift) & 0x0f;
    if (new_val > old_val) {
      mask &= (0xffffffu - (0x0fu << shift));
      mask |= new_val << shift;
    }
  }
  return mask;
}
