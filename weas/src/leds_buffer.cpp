
#include "weas/leds_buffer.h"

#include "weas/leds.h"


void LEDsBuffer::lazy_start_on_local_core() {
  if (!alarm_pool) {
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
