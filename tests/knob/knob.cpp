
#include "pico/stdlib.h"

#include "weas/led.h"
#include "weas/codec.h"


int main() {

  auto& leds = LED::get();
  leds.display7levels(1);
  auto& codec = Codec<SAMPLE_44_1K, 0>::get();
  codec.set_callback([&codec, &leds]() mutable {
    leds.display7levels(5);
    const uint16_t k = codec.get_knob(Knob::Main);
    leds.display12bits(k);
  });
  codec.start_irq();
  // leds.display7levels(6);
  while (1) sleep_ms(10000);
}
