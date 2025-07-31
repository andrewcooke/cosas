
#include <cmath>

#include "weas/codec.h"
#include "weas/leds.h"
#include "weas/fifo.h"

#include "weas/leds_mask.h"

constexpr float FDIV = 1.0f;
constexpr uint SAMPLE_FREQ = static_cast<uint>(CC_SAMPLE_44_1 / FDIV);

typedef Codec<1, SAMPLE_FREQ> CC_;


class FIFODemo : public KnobChanges {
public:

  FIFODemo() : cc(CC_::get()) {};

  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t) override {
    if (knob == 0) {
      uint32_t ring = mask.ring(static_cast<float>(now) / 4095, abs(now - 2048) < 16 || now < 8 || now > 4087);
      mask.show(mask.modulate(ring, top_square, cc.get_count() >> 14));
      // mask.show(ring);
      // mask.show(top_square);
    }
  }

private:

  LEDsMask mask;
  uint32_t top_square = mask.top_square(0x05);
  CC_& cc;
};

int main() {
  FIFODemo demo;
  auto& fifo = FIFO<1, SAMPLE_FREQ>::get();
  fifo.set_knob_changes(&demo);
  fifo.start();
  CC_::get().start();
};

