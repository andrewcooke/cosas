
#include "pico/time.h"
#include "pico/types.h"
#include "weas/leds.h"

int main() {
    LEDs& leds = LEDs::get();
    while (true) {
        for (uint led = 0; led < 6; led++) {
            for (uint8_t b = 0; b < 0xff; b++) {
                leds.set(led, b);
                sleep_ms(2);
            }
            for (uint8_t b = 0xff; b > 0; b--) {
                leds.set(led, b);
                sleep_ms(1);
            }
        }
        for (uint16_t n = 0; n < 0xfff; n++) {
            leds.display12bits(n);
            sleep_ms(1);
        }
        for (uint delay = 100; delay > 2; delay = 0.75 * delay) {
            for (uint c = 0; c < 2; c++) {
                for (uint8_t n = 0; n < 10; n++) {
                    leds.column10levels(c, n);
                    sleep_ms(delay);
                }
            }
        }
        leds.all(false);
    }
}
