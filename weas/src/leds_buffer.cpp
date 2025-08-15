
#include <bitset>

#include "weas/leds_buffer.h"

#include "weas/debug.h"
#include "weas/leds_mask.h"


void LEDsBuffer::lazy_start_on_local_core() {
  if (!alarm_pool && !DISABLED) {
    alarm_pool = alarm_pool_create_with_unused_hardware_alarm(1);
    alarm_pool_add_repeating_timer_ms(alarm_pool, TIMER_MS, &trampoline, this, &out);
  }
}

LEDsBuffer& LEDsBuffer::get() {
  static LEDsBuffer leds_buffer;
  return leds_buffer;
}

bool LEDsBuffer::trampoline(repeating_timer_t *rt) {
  static_cast<LEDsBuffer*>(rt->user_data)->render();
  return true;
}

void LEDsBuffer::render() {
  while (!buffer.empty()) {
    auto next = buffer.front();
    // Debug::log(std::bitset<30>(std::get<0>(next)));
    LEDsMask::show(LEDs::get(), std::get<0>(next));
    buffer.pop();
    if (std::get<1>(next)) {
      Debug::log("hold");
      return;
    }
  }
}

uint32_t LEDsBuffer::get_mask() {return mask;}

void LEDsBuffer::queue(uint32_t mask_new, bool force, bool interp, uint n) {
  if (DISABLED) {
    Debug::log("direct [", std::bitset<30>(mask_new), "]");
    LEDsMask::show(LEDs::get(), mask_new);
  } else {
    lazy_start_on_local_core();
    if (buffer.empty() && !interp && !n && mask == mask_new) return;
    if (force) std::queue<std::tuple<uint32_t, bool>>().swap(buffer);
    if (interp) {
      for (uint weight = 0; weight < INTERP_N - 1; ++weight) {
        uint32_t old = mask;
        uint32_t new_ = mask_new;
        uint32_t between = 0;
        for (uint led = 0; led < LEDs::N; led++) {
          uint32_t b0 = old & LEDsMask::BITS_MASK;
          uint32_t b1 = new_ & LEDsMask::BITS_MASK;
          between |= LEDsMask::BITS_MASK & ((weight * b1 + (INTERP_N - weight) * b0) >> INTERP_BITS);
          old >>= LEDsMask::BITS;
          new_ >>= LEDsMask::BITS;
          between <<= LEDsMask::BITS;
        }
        buffer.push({LEDsMask::reverse(between), true});
      }
    }
    for (uint i = 0; i < n; ++i) {buffer.push({mask_new, true});}
    buffer.push({mask_new, false});
  }
}
