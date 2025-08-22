
#ifndef COSAS_APP_H
#define COSAS_APP_H


#include <cstdint>

#include "cosas/common.h"
#include "cosas/knobs.h"
#include "cosas/source.h"


class App {

public:

  virtual ~App() = default;
  virtual uint8_t get_n_sources() = 0;
  virtual RelSource& get_source(uint8_t source) = 0;
  virtual uint8_t get_n_pages(uint8_t source) = 0;
  virtual KnobHandler get_knob(uint8_t page, Knob knob) = 0;
  virtual void set_source(uint8_t source) = 0;

};


#endif

