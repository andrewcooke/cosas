
#ifndef COSAS_APP_FOME_H
#define COSAS_APP_FOME_H


#include "cosas/app.h"
#include "cosas/engine_small.h"


class FomeApp : public App {

public:
  FomeApp();
  uint8_t n_sources() override;
  RelSource* get_source(uint8_t s) override;
  uint8_t n_pages() override;
  Param& get_param(uint8_t page, Knob knob) override;

private:
  SmallManager manager;
  uint8_t source_idx;
  RelSource* source = nullptr;
};


#endif

