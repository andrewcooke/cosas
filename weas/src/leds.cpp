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

// todo - left to right
void LEDs::display12bits(const uint16_t v) {
  uint16_t vv = v & 0x0fff;
  for (uint i = N; i > 0; i--) {
    set(i - 1, static_cast<uint8_t>((vv & 0x03) << 6));
    vv >>= 2;
  }
}

void LEDs::display12bits(const int16_t v) {
  display12bits(static_cast<uint16_t>(v + 0x8000));
}

void LEDs::column10levels(const uint c, uint8_t v) {
  column10levels(true, c, v);
}

void LEDs::column3levels(const uint c, const uint8_t v) {
  for (uint i = 0; i < N / 2; i++) {
    set(4 - 2 * i + (c & 0x1), i == v);
  }
}

void LEDs::display7levels(const uint8_t n) {
  for (uint i = 0; i < N; i++) set(i, i < n);
}

void LEDs::columns12bits(uint16_t v) {
  v = v & 0x0fff;
  uint8_t v1 = v / 409;
  uint8_t v2 = (v % 409) / 41;
  column10levels(0, v1);
  column10levels(1, v2);
}

void LEDs::columns12bits(int16_t v) {
  if (v >= 0) {
    columns11bits(true, v);
  } else {
    columns11bits(false, -v);
  }
}

void LEDs::columns11bits(bool up, uint16_t v) {
  v = v & 0x07ff;
  uint8_t v1 = v / 204;
  uint8_t v2 = (v % 204) / 21;
  column10levels(up, 0, v1);
  column10levels(up, 1, v2);
}

void LEDs::column10levels(const bool up, const uint c, uint8_t v) {
  v = std::min(static_cast<uint8_t>(9), v);
  for (uint i = 0; i < N / 2; i++) {
    uint led = up ? 4 + (c & 0x1) - 2 * i : (c & 0x1) + 2 * i;
    set(led, static_cast<uint8_t>(std::min(static_cast<uint8_t>(3), v) << 6));
    v = v > 3 ? v - 3 : 0;
  }
}

void LEDs::display13levels(uint8_t v) {
  v = std::min(static_cast<uint8_t>(12), v);
  const bool inv = v > 6;
  if (inv) v -= 6;
  for (uint i = 0; i < N; i++) {
    set(i, (i == v) != inv);
  }
}

void LEDs::sq4(const uint n, const uint8_t v) {
  for (uint i = 0; i < 4; i++) {
    set(i + (n & 1) * 2, v);
  }
}

void LEDs::v2(const uint n, const uint8_t v) {
  set(n & 3, v);
  set((n & 3) + 2, v);
}

void LEDs::v3(const uint n, uint8_t v) {
  set(n & 1, v);
  set((n & 1) + 2, v);
  set((n & 1) + 4, v);
}

void LEDs::h2(uint n, uint8_t v) {
  n = n % 3;
  set(n * 2, v);
  set(n * 2 + 1, v);
}

auto LEDs::display7bits(const int16_t v) -> void {
  const bool neg = v < 0;
  uint16_t vv = abs(v) & 0x3f;
  for (uint i = 0; i < N; i++) {
    if (vv & 1) set(i, neg ? 0x20 : 0xffu);
    else set(i, 0x00u);
    vv >>= 1;
  }
}
