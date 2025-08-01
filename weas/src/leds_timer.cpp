
#include "weas/leds_timer.h"


LEDsTimer::LEDsTimer(Codec& codec) : LEDsTimer(codec, 100) {};

LEDsTimer::LEDsTimer(Codec& codec, uint freq) : codec(codec), freq(freq), mask(0), extra(0) {
  add_repeating_timer_ms(static_cast<int32_t>(1000 / freq), &trampoline, this, &out);
}

bool LEDsTimer::trampoline(repeating_timer_t *rt) {
  static_cast<LEDsTimer*>(rt->user_data)->render();
  return true;
}

void LEDsTimer::render() {
  leds_mask->show(mask, extra, codec.get_count() >> 12);
}

void LEDsTimer::show(uint32_t mask) {
  show(mask, 0u);
}

void LEDsTimer::show(uint32_t mask_, uint32_t extra_) {
  mask = mask_;
  extra = extra_;
}
