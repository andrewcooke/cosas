
#include "pico/stdlib.h"

#include "weas/led.h"
#include "weas/codec.h"


int main() {

  auto& codec = Codec<SAMPLE_44_1K, 1>::get();
  auto& leds = LED::get();
  codec.set_callback([&codec, &leds]() mutable {
    const uint16_t k = codec.get_knob(Knob::Main);
    leds.display12bits(k);
  });

}
