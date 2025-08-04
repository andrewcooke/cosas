
#include <utility>

#include "weas/leds_timer.h"

#include <algorithm>

LEDsTimer::LEDsTimer(Codec& codec) : codec(codec), mask(0), extra(0) {};

void LEDsTimer::lazy_start_on_local_core() {
  if (! alarm_pool) {
    alarm_pool = alarm_pool_create_with_unused_hardware_alarm(1);
    alarm_pool_add_repeating_timer_ms(alarm_pool, 1000 / TIMER_HZ, &trampoline, this, &out);
  }
}

LEDsTimer &LEDsTimer::get(Codec &codec) {
  static LEDsTimer leds_timer(codec);
  return leds_timer;
}

void LEDsTimer::set_speed(uint s) {speed = s;}

bool LEDsTimer::trampoline(repeating_timer_t *rt) {
  static_cast<LEDsTimer*>(rt->user_data)->render();
  return true;
}

void LEDsTimer::render() {
  if (!loop_data.empty()) load_loop();
  leds_mask->show(mask, extra, codec.get_count() >> speed);
}

void LEDsTimer::show(uint32_t mask) {
  show(mask, 0u);
}

void LEDsTimer::show(uint32_t mask_, uint32_t extra_) {
  mask = mask_;
  extra = extra_;
  lazy_start_on_local_core();
}

void LEDsTimer::loop(int scale, int limit, std::vector<std::tuple<uint32_t, uint32_t>> mex) {
  loop_scale = scale;
  loop_limit = limit;
  loop_data = std::move(mex);
  loop_zero = codec.get_count() >> loop_scale;
}

void LEDsTimer::load_loop() {
  uint count = (codec.get_count() >> loop_scale) - loop_zero;
  if (count < loop_limit) {
    uint idx = count % loop_data.size();
    show(std::get<0>(loop_data[idx]), std::get<1>(loop_data[idx]));
  } else if (count == loop_limit) {
    clear_loop();
  }
}

void LEDsTimer::clear_loop() {
  loop_data.clear();
}