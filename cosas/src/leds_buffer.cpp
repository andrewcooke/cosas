
#include "cosas/leds_buffer.h"


void BaseLEDsBuffer::render() {
  while (!buffer.empty()) {
    auto next = buffer.front();
    mask = std::get<0>(next);
    leds_mask->show(mask);
    buffer.pop();
    if (std::get<1>(next)) return;
  }
}

uint32_t BaseLEDsBuffer::get_mask() {return mask;}

void BaseLEDsBuffer::queue(uint32_t mask_new, bool force, bool interp, size_t n) {
  // TODO - lock buffer
  lazy_start_on_local_core();
  if (buffer.empty() && !interp && !n && mask == mask_new) return;
  if (force) std::queue<std::tuple<uint32_t, bool>>().swap(buffer);
  if (interp) {
    uint32_t mask_old = buffer.empty() ? mask : std::get<0>(buffer.back());
    for (size_t weight = 0; weight < INTERP_N; weight++) {
      uint32_t old = mask_old;
      uint32_t new_ = mask_new;
      uint32_t between = 0;
      for (size_t led = 0; led < BaseLEDsMask::N; led++) {
        uint32_t b0 = old & leds_mask->BITS_MASK;
        uint32_t b1 = new_ & leds_mask->BITS_MASK;
        between <<= leds_mask->BITS;
        between |= leds_mask->BITS_MASK & ((weight * b1 + (INTERP_N - weight) * b0) >> INTERP_BITS);
        old >>= leds_mask->BITS;
        new_ >>= leds_mask->BITS;
      }
      buffer.push({leds_mask->reverse(between), true});
    }
  }
  for (size_t i = 0; i < n; ++i) {buffer.push({mask_new, true});}
  buffer.push({mask_new, false});
}
