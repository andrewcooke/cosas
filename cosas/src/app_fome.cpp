

#include "cosas/app_fome.h"
#include "cosas/debug.h"


FomeApp::FomeApp()
  : source_idx(0), source(&manager.build(static_cast<SmallManager::SmallEngine>(source_idx))) {};

uint8_t FomeApp::n_sources() {
  return SmallManager::N_ENGINE;
}

RelSource* FomeApp::get_source(uint8_t s) {
  if (s != source_idx) {
    source_idx = s;
    source = &manager.build(static_cast<SmallManager::SmallEngine>(s));
  }
  return source;
}

uint8_t FomeApp::n_pages() {
  return manager.n_panes();
}

Param& FomeApp::get_param(uint8_t page, Knob knob) {
  return manager.get_pane(page).get_param(knob);
}

TapMixin& FomeApp::get_tap(uint8_t page) {
  return manager.get_pane(page).tap;
}
