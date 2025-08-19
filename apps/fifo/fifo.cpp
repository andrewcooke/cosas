
#include "weas/debug.h"
#include "weas/fifo.h"
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
  codec.start();
};
