

#include "cosas/app_fome.h"


uint8_t FomeApp::get_n_sources() {
  return 3;
}

class FomeSource : public RelSource {

public:
  int16_t next(int32_t /* delta */, int32_t /* phi */) {
    return 0;
  }

};

RelSource& FomeApp::get_source(uint8_t /* source */) {
  static FomeSource dummy_source;
  return dummy_source;
}

uint8_t FomeApp::get_n_pages(uint8_t /* source */) {
  return 3;
}

KnobHandler FomeApp::get_knob(uint8_t /* page */, Knob /* knob */) {
  static KnobHandler knob(1, 0, false, 0, 1);
  return knob;
}
