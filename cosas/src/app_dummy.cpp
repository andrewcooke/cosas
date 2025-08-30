

#include "cosas/app_dummy.h"


uint8_t DummyApp::n_sources() {
  return 19;
}

class DummySource : public RelSource {

public:
  int16_t next(int32_t /* delta */, int32_t /* phi */) {
    return 0;
  }

};

RelSource* DummyApp::get_source(uint8_t /* source */) {
  static DummySource dummy_source;
  return &dummy_source;
}

uint8_t DummyApp::n_pages() {
  return 3;
}

Param& DummyApp::get_param(uint8_t /* page */, Knob /* knob */) {
  static DummyParam param = DummyParam(1, 0, false, 0, 1);
  return param;
}
