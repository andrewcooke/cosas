
#include "weas/codec.h"
#include "weas/fifo.h"
#include "weas/knobs.h"
#include "weas/ui_state.h"


typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;

int main() {
  auto& codec = CC_::get();
  // codec.set_adc_mask(Codec::Knobs, 0xff0);
  Knob knob;
  UIState demo(codec, knob);
  auto& fifo = FIFO::get();
  fifo.set_knob_changes(&demo);
  fifo.start(codec);
  codec.start();
};
