
// ReSharper disable once CppUnusedIncludeDirective
#include "pico/stdlib.h"  // uint

#include "weas/codec.h"
#include "weas/leds.h"

class XX: public Codec {

  void ProcessSample() override {
    // LED::get().display12bits(static_cast<uint16_t>((get_count() >> 12) + 1));
    // LED::get().columns12bits(static_cast<uint16_t>(KnobVal(Main)));  // x is broken
    // LED::get().column3levels(0, SwitchVal());
    // LED::get().columns12bits(CVIn(1));
    LEDs::get().columns12bits(AudioIn(0));
  }

};

int main() {
  auto& leds = LEDs::get();
  // auto& codec = Codec<SAMPLE_44_1K, 0>::get();
  // codec.set_callback([&codec, &leds]() mutable {
  //   leds.display12bits(static_cast<uint16_t>((codec.get_count() >> 12) + 1));
  //   // leds.columns12bits(codec.get_knob(Y));
  //   // leds.columns12bits(codec.get_cv(1));
  //   // leds.columns12bits(codec.get_adc(1));
  //   // leds.column3levels(0, codec.get_switch());
  //   // leds.column3levels(1, 0);
  // });
  // codec.start_irq(true);
  XX().Run();
}


