
#include "weas/fifo.h"
#include "weas/debug.h"

#include "cosas/dnl.h"
#include "cosas/app_fome.h"

#include "weas/codec.h"
#include "weas/ui_state.h"


int main() {
  Debug::get().init();
  auto& codec = CodecFactory<1, CODEC_SAMPLE_44_1>::get();
  Debug::log(1);
  auto& fifo = FIFO::get();
  Debug::log(2);
  FomeApp app;
  Debug::log(3);
  UIState ui(app, codec.read_switch());
  Debug::log(4);
  fifo.set_ctrl_changes(&ui);
  Debug::log(5);
  fifo.start(codec);
  Debug::log(6);
  codec.set_adc_correction_and_scale(fix_dnl);
  Debug::log(7);
  codec.start();
  Debug::log(8);
};
