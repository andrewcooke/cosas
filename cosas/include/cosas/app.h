
#ifndef COSAS_APP_H
#define COSAS_APP_H


#include <cstdint>

#include "cosas/common.h"
#include "cosas/knobs.h"
#include "cosas/source.h"


// this assumes that creating a source may be expensive but accessing
// a pane for teh current source is cheap  so source is internal state,
// but pane is not.

class App {

public:

  virtual ~App() = default;
  virtual uint8_t n_sources() = 0;
  virtual RelSource& get_source(uint8_t source) = 0;
  virtual uint8_t n_pages() = 0;
  virtual Param& get_param(uint8_t page, Knob knob) = 0;

};


#endif

