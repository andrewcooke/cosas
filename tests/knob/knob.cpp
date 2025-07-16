
// ReSharper disable once CppUnusedIncludeDirective
#include "pico/stdlib.h"  // uint

#include "weas/led.h"
#include "weas/codec.h"


int main() {

  auto& leds = LED::get();
  auto& codec = Codec<SAMPLE_44_1K, 1>::get();
  codec.set_callback([&codec, &leds]() mutable {
    // leds.columns12bits(codec.get_knob(Main));
    leds.columns12bits(codec.get_cv(0) >> 4);
  });
  codec.start_irq(true);
}
