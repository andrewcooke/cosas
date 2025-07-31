
#include <algorithm>
#include <cmath>
#include <numbers>

#include "cosas/dnl.h"
#include "weas/codec.h"
#include "weas/leds.h"
#include "weas/eeprom.h"
#include "weas/fifo.h"

#include "weas/leds_mask.h"

constexpr float FDIV = 100.0f;
constexpr uint SAMPLE_FREQ = static_cast<uint>(CC_SAMPLE_44_1 / FDIV);

typedef Codec<1, SAMPLE_FREQ> CC_;


class FIFODemo : public KnobChanges {

public:
  FIFODemo() = default;
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t) override {
    if (knob == 0) {
      mask.show(mask.ring(static_cast<float>(now) / 4095, abs(now - 2048) < 16));
    }
  }

private:
  LEDsMask mask;

};

int main() {
  FIFODemo demo;
  auto& cc = CC_::get();
  auto& fifo = FIFO<1, SAMPLE_FREQ>::get();
  fifo.set_knob_changes(&demo);
  fifo.start();
  cc.start();
};

