
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
