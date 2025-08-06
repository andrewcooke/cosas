
#include <cmath>

#include "weas/codec.h"
#include "weas/fifo.h"
#include "weas/leds.h"
#include "weas/leds_direct.h"
#include "weas/leds_mask.h"
#include "weas/leds_timer.h"


typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;


class FIFODemo : public KnobChanges {
public:

  explicit FIFODemo(Codec& codec) : leds_timer(LEDsTimer::get(codec)) {};

  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t /* prev */) override {
    uint32_t ring = 0;
    switch (knob) {
    case (Codec::Main):
    case (Codec::X):
    case (Codec::Y):
      ring = leds_mask.ring(static_cast<float>(now) / 4095,
        abs(now - 2048) < 16 || now < 8 || now > 4087);
      leds_timer.show(ring, overlay[knob]);
      // LEDsDirect().display12bits(now);
      break;
    case (Codec::Switch):
      if (now == Codec::Down) {
        leds_timer.loop(12, 1, {
          {leds_timer.get_mask(), leds_timer.get_extra()},
          {leds_timer.get_mask(), leds_timer.get_extra()}});
      } else if (now == Codec::Up) {
        leds_timer.clear_loop();
      }
    }
  }

private:

  static constexpr uint8_t amp = 0x6;
  LEDsDirect leds;
  LEDsMask leds_mask;
  LEDsTimer& leds_timer;
  // afaict the "3" should not be needed here, but i get an error without it.
  uint32_t overlay[3] = {
    leds_mask.square(0, amp),
    leds_mask.vbar(0, amp),
    leds_mask.square(1, amp)};
};

int main() {
  auto& codec = CC_::get();
  // codec.set_adc_mask(Codec::Knobs, 0xff0);
  FIFODemo demo(codec);
  auto& fifo = FIFO::get();
  fifo.set_knob_changes(&demo);
  fifo.start(codec);
  codec.start();
};

