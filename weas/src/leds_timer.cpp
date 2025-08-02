
#include "weas/leds_timer.h"


LEDsTimer::LEDsTimer(Codec& codec) : codec(codec), mask(0), extra(0) {
  alarm_pool = alarm_pool_create_with_unused_hardware_alarm(1);
  alarm_pool_add_repeating_timer_ms(alarm_pool, 1000 / TIMER_HZ, &trampoline, this, &out);
}

LEDsTimer &LEDsTimer::get(Codec &codec) {
  static LEDsTimer leds_timer(codec);
  return leds_timer;
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
