

#include "cosas/app.h"


KnobSpec::KnobSpec(float lo, float hi, bool log, void (*callback)(float), float scale, float linearity)
  : lo(lo), hi(hi), log(log), callback(callback), scale(scale), linearity(linearity) {};


uint8_t DummyApp::get_n_sources() {
  return 3;
}

class DummySource : public RelSource {

public:
  int16_t next(int32_t /* delta */, int32_t /* phi */) {
    return 0;
  }

};

RelSource& DummyApp::get_source(uint8_t /* source */) {
  static DummySource dummy_source;
  return dummy_source;
}

uint8_t DummyApp::get_n_pages(uint8_t /* source */) {
  return 3;
}

KnobSpec DummyApp::get_knob_spec(uint8_t /* page */, KnobSpec::Knob /* knob */) {
  static KnobSpec knob_spec(0, 1, false, [](float /* x */) -> void {}, 1, 0);
  return knob_spec;
}
