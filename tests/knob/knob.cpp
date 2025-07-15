
#include "pico/stdlib.h"

#include "weas/led.h"
#include "weas/codec.h"


int main() {

  auto& leds = LED::get();
  leds.display7levels(1);
  auto& codec = Codec<SAMPLE_44_1K, 1>::get();
  codec.set_callback([&codec, &leds]() mutable {
    leds.display7levels(5);
    const uint16_t k = codec.get_adc(1);
    leds.display12bits(k);
  });
  codec.start_irq(true);
  leds.display7levels(6);
}
