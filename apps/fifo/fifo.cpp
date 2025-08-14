
#include "weas/codec.h"
#include "weas/leds_timer.h"
#include "weas/fifo.h"
#include "weas/ui_state.h"


typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;

int main() {
  auto& codec = CC_::get();
  auto& leds_timer = LEDsTimer::get(codec);
  UIState ui(leds_timer);
  auto& fifo = FIFO::get();
  fifo.set_knob_changes(&ui);
  fifo.start(codec);
  codec.start();
};
