
#include <algorithm>
#include <cmath>
#include <numbers>

#include "cosas/dnl.h"
#include "weas/codec.h"
#include "weas/leds.h"
#include "weas/eeprom.h"
#include "weas/fifo.h"

constexpr float FDIV = 44.1f;
constexpr uint SAMPLE_FREQ = static_cast<uint>(CC_SAMPLE_44_1 / FDIV);

typedef Codec<1, SAMPLE_FREQ> CC_;


class FIFODemo : public KnobChanges {

public:
  FIFODemo() = default;
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t) override {
    if (knob == 0) {
      leds.columns12bits(now);
    }
  }

private:
  LEDs& leds = LEDs::get();

};

int main() {
  FIFODemo demo;
  auto& cc = CC_::get();
  auto& fifo = FIFO<1, SAMPLE_FREQ>::get();
  fifo.set_knob_changes(&demo);
  fifo.start();
  cc.start();
};

