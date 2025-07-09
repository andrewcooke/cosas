
#include <algorithm>

#include "weas/led.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"


LED::LED() {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 0xffff);  // input is 8bit, but we square
    uint prev_slice = 0;
    for (size_t index = 0; index < LED_N; index++) {
        uint gpio = LED_BASE_GPIO + index;
        gpio_set_function(gpio, GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(gpio);
        if (index == 0 || slice != prev_slice) {  // they come in pairs?
            pwm_init(slice, &config, true);
            prev_slice = slice;
        }
        pwm_set_gpio_level(gpio, 0);
    }
}

LED& LED::get() {
    static LED instance;
    return instance;
}

// this could be static, but maybe in the future we have more stateful behaviour
void LED::set(const uint index, const uint8_t b) { // NOLINT(*-convert-member-functions-to-static)
    const uint16_t b2 = b * b;
    pwm_set_gpio_level(LED_BASE_GPIO + std::min(static_cast<uint>(5), index), b2);
}

void LED::set(const uint index, bool x) { // NOLINT(*-convert-member-functions-to-static)
    set(index, static_cast<uint8_t>(x ? 0xff : 0));
}

void LED::all(bool x) {
    for (uint i = 0; i < LED_N; i++) {set(i, x);}
}

void LED::all(uint8_t b) {
    for (uint i = 0; i < LED_N; i++) {set(i, b);}
}

void LED::display12bits(uint16_t v) {
    v &= 0x0fff;
    for (uint i = LED_N; i > 0; i--) {
        set(i - 1, static_cast<uint8_t>((v & 0x03) << 6));
        v >>= 2;
    }
}

void LED::display12bits(int16_t v) {
    display12bits(static_cast<uint16_t>(v + 0x8000));
}

void LED::column10levels(const uint c, uint8_t v) {
    v = v % 10;
    for (int i = 5 + (c & 1); i > 0; i -= 2) {
        set(i - 1, static_cast<uint8_t>(std::min(static_cast<uint8_t>(3), v) << 6));
        v = v > 3 ? v - 3 : 0;
    }
}

void LED::display7levels(uint8_t n) {
    for (uint i = 0; i < LED_N; i++) set(i, i < n);
}

