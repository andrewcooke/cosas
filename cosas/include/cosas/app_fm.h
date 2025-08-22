
#ifndef COSAS_APP_FM_H
#define COSAS_APP_FM_H


#include "cosas/app.h"


class FMApp : public App {

public:

  uint8_t get_n_sources() override;
  RelSource& get_source(uint8_t source) override;
  uint8_t get_n_pages(uint8_t source) override;
  KnobHandler get_knob(uint8_t page, Knob knob) override;
  void set_source(uint8_t source) override;

};


#endif

