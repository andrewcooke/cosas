
#include <cmath>

#include "weas/codec.h"
#include "weas/leds.h"
#include "weas/fifo.h"

#include "weas/leds_mask.h"

#include <weas/leds_timer.h>

typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;


class FIFODemo : public KnobChanges {
public:

  FIFODemo() : leds_timer(std::make_unique<LEDsTimer>(CC_::get(), 100)), cc(CC_::get()) {}

  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t) override {
    if (knob == 0) {
      uint32_t ring = mask.ring(static_cast<float>(now) / 4095,
        abs(now - 2048) < 16 || now < 8 || now > 4087);
      leds_timer->show(ring, square);
    }
  }

private:

  LEDsMask mask;
  std::unique_ptr<LEDsTimer> leds_timer;
  uint32_t square = mask.top_square(0x7);
  CC_& cc;
};

int main() {
  FIFODemo demo;
  auto& fifo = FIFO::get();
  auto& cc = CC_::get();
  // cc.set_adc_mask(CC_::Knobs, 0xfff0);  // discard bottom 4 bits
  fifo.set_knob_changes(&demo);
  fifo.start(cc);
  cc.start();
};

