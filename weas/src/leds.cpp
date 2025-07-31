#include <algorithm>

#include "weas/leds.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"


LEDs::LEDs() {
  pwm_config config = pwm_get_default_config();
  pwm_config_set_wrap(&config, 0xffff); // input is 8bit, but we square
  uint prev_slice = 0;
  for (size_t index = 0; index < N; index++) {
    uint gpio = LED_BASE_GPIO + index;
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    if (index == 0 || slice != prev_slice) {
      // they come in pairs?
      pwm_init(slice, &config, true);
      prev_slice = slice;
    }
    pwm_set_gpio_level(gpio, 0);
  }
}

LEDs& LEDs::get() {
  static LEDs instance;
  return instance;
}

// this could be static, but maybe in the future we have more stateful behaviour
void LEDs::set(const uint index, const uint8_t b) { // NOLINT(*-convert-member-functions-to-static)
  const uint16_t b2 = b * b;
  pwm_set_gpio_level(LED_BASE_GPIO + std::min(static_cast<uint>(5), index), b2);
}

// alias for literals
void LEDs::set(const uint index, const uint b) {
  set(index, static_cast<uint8_t>(b & 0xff));
}

void LEDs::set(const uint index, bool x) { // NOLINT(*-convert-member-functions-to-static)
  set(index, static_cast<uint8_t>(x ? 0xff : 0));
}

void LEDs::on(const uint index) {
  set(index, true);
}

void LEDs::off(const uint index) {
  set(index, false);
}

void LEDs::all(const bool x) {
  for (uint i = 0; i < N; i++) { set(i, x); }
}

void LEDs::all(const uint8_t b) {
  for (uint i = 0; i < N; i++) { set(i, b); }
}

// alias for literal integers
void LEDs::all(const uint b) {
  all(static_cast<uint8_t>(b & 0xff));
}
