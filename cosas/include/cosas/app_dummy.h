
#ifndef COSAS_APP_DUMMY_H
#define COSAS_APP_DUMMY_H


#include "cosas/app.h"


class DummyApp : public App {

public:

  uint8_t n_sources() override;
  RelSource& get_source(uint8_t source) override;
  uint8_t n_pages() override;
  Param& get_param(uint8_t page, Knob knob) override;

};


#endif

