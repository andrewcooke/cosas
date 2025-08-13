
#include "weas/codec.h"
#include "weas/fifo.h"
#include "weas/knobs.h"
#include "weas/ui_state.h"


typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;

int main() {
  auto& codec = CC_::get();
  UIState demo(codec);
  auto& fifo = FIFO::get();
  fifo.set_knob_changes(&demo);
  fifo.start(codec);
  codec.start();
};
