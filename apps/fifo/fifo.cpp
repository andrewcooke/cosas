
#include "weas/fifo.h"
#include "weas/debug.h"

#include "cosas/dnl.h"
#include "weas/codec.h"
#include "weas/ui_state.h"


typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;

int main() {
  // Debug::init();
  auto& codec = CC_::get();
  auto& fifo = FIFO::get();
  DummyApp app;
  UIState ui(app, codec.read_switch());
  fifo.set_knob_changes(&ui);
  fifo.start(codec);
  codec.set_adc_correction_and_scale(fix_dnl);
  codec.start();
};
