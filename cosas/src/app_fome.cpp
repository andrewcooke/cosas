

#include "cosas/app_fome.h"


FomeApp::FomeApp() : manager(), source(manager.build(static_cast<Manager::Engine>(0))) {};

uint8_t FomeApp::n_sources() {
  return Manager::N_ENGINE;
}

RelSource& FomeApp::get_source(uint8_t s) {
  source = manager.build(static_cast<Manager::Engine>(s));
  return source;
}

uint8_t FomeApp::n_pages() {
  return manager.n_panes();
}

KnobHandler FomeApp::get_knob(uint8_t page, Knob knob) {
  return ParamHandler(manager.get_pane(page).get_param(knob));
}
