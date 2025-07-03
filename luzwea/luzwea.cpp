
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
    }
}
