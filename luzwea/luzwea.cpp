
#include "pico/types.h"
#include "weas/leds.h"

int main() {
    while (true) {
        for (uint led = 0; led < 6; led++) {
            for (uint16_t b = 0; b < 0xffff; b++) {
                LEDs::get().set(led, b);
            }
            for (uint16_t b = 0xffff; b > 0; b--) {
                LEDs::get().set(led, b);
            }
        }
    }
}
