
#ifndef COSAS_APP_FOME_H
#define COSAS_APP_FOME_H


#include "cosas/app.h"
#include "cosas/engine.h"


class FomeApp : public App {

public:
  FomeApp();
  uint8_t n_sources() override;
  RelSource& get_source(uint8_t s) override;
  uint8_t n_pages() override;
  KnobHandler& get_knob(uint8_t page, Knob knob) override;

private:
  std::vector<std::array<std::unique_ptr<ParamHandler>, N_KNOBS>> knobs;
  Manager manager;
  RelSource& source;
};


#endif

