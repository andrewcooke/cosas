
#include "weas/debug.h"
#include "weas/fifo.h"
#include "weas/codec.h"
#include "weas/ui_state.h"


typedef CodecFactory<1, CODEC_SAMPLE_44_1> CC_;

int main() {
  Debug::init();
  auto& fifo = FIFO::get();
  UIState ui;
  Debug::log(1);
  fifo.set_knob_changes(&ui);
  Debug::log(2);
  auto& codec = CC_::get();
  Debug::log(3);
  fifo.start(codec);
  Debug::log(4);
  codec.start();
  Debug::log(5);
};
