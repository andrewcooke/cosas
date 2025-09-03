
#include "RP2040Atomic.hpp"

#include "weas/fifo.h"
#include "weas/debug.h"

#include "cosas/dnl.h"
#include "cosas/app_fome.h"
#include "cosas/app_dummy.h"

#include "weas/codec.h"
#include "weas/ui_state.h"


int main() {
  // Debug::get().init();
  patom::PseudoAtomicInit();
  auto& codec = CodecFactory<1, CODEC_SAMPLE_44_1>::get();
  codec.set_ctrl_alpha(1);
  auto& fifo = FIFO::get();
  FomeApp app;
  UIState ui(app, fifo, codec.read_switch());
  codec.set_per_sample_cb([&ui](Codec& c) {ui.per_sample_cb(c);});
  fifo.set_ctrl_changes(&ui);
  fifo.start(codec);
  codec.set_adc_correction_and_scale(fix_dnl);
  codec.start();
};
