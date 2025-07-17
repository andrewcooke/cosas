
// ReSharper disable once CppUnusedIncludeDirective
#include "pico/stdlib.h"  // uint

#include "weas/led.h"
#include "weas/codec.h"


int main() {

  auto& leds = LED::get();
  auto& codec = Codec<SAMPLE_44_1K, 0>::get();
  codec.set_callback([&codec, &leds]() mutable {
    leds.display12bits(static_cast<uint16_t>(codec.get_count() + 1));
    // leds.columns12bits(codec.get_knob(Y));
    // leds.columns12bits(codec.get_cv(1));
    // leds.columns12bits(codec.get_adc(1));
    // leds.column3levels(0, codec.get_switch());
    // leds.column3levels(1, 0);
  });
  codec.start_irq(true);
}
