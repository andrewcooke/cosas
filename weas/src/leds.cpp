
#include <algorithm>

#include "weas/leds.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"


LEDs::LEDs() {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 0xffff);  // input is 8bit, but we square
    for (size_t index = 0; index < 6; index++) {
        uint gpio = BASE_GPIO + index;
        gpio_set_function(gpio, GPIO_FUNC_PWM);
        // i think this is being called twice as often as it need be?
        uint slice = pwm_gpio_to_slice_num(gpio);
        pwm_init(slice, &config, true);
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
    pwm_set_gpio_level(BASE_GPIO + std::min(static_cast<uint>(5), index), b2);
}

void LEDs::set(const uint index, bool x) { // NOLINT(*-convert-member-functions-to-static)
    set(index, static_cast<uint8_t>(x ? 0xff : 0));
}

void LEDs::all(bool x) {
    for (uint i = 0; i < 6; i++) {set(i, x);}
}

void LEDs::all(uint8_t b) {
    for (uint i = 0; i < 6; i++) {set(i, b);}
}

void LEDs::display12bits(uint16_t v) {
    v &= 0x0fff;
    for (uint i = 6; i > 0; i--) {
        set(i - 1, static_cast<uint8_t>((v & 0x03) << 6));
        v >>= 2;
    }
}

void LEDs::display12bits(int16_t v) {
    display12bits(static_cast<uint16_t>(v + 0x8000));
}

void LEDs::column10levels(const uint c, uint8_t v) {
    v = v % 10;
    for (int i = 5 + (c & 1); i > 0; i -= 2) {
        set(i - 1, static_cast<uint8_t>(std::min(static_cast<uint8_t>(3), v) << 6));
        v = v > 3 ? v - 3 : 0;
    }
}
