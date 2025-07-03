
#include <algorithm>

#include "weas/leds.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"


LEDs::LEDs() {
    pwm_config config = pwm_get_default_config();
    for (size_t index = 0; index < 6; index++) {
        uint gpio = BASE_GPIO + index;
        gpio_set_function(gpio, GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(gpio);
        pwm_init(slice, &config, true);
        pwm_set_gpio_level(gpio, 0);
    }
}

LEDs& LEDs::get() {
    static LEDs instance;
    return instance;
}

void LEDs::set(uint index, uint8_t brightness) {
    pwm_set_gpio_level(BASE_GPIO + std::min(static_cast<uint>(5), index), brightness);
}
