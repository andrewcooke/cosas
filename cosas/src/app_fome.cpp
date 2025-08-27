

#include "cosas/app_fome.h"


FomeApp::FomeApp() : manager(), source(manager.build(static_cast<SmallManager::SmallEngine>(0))) {};

uint8_t FomeApp::n_sources() {
  return SmallManager::N_ENGINE;
}

RelSource& FomeApp::get_source(uint8_t s) {
  source = manager.build(static_cast<SmallManager::SmallEngine>(s));
  knobs = std::vector<std::array<std::unique_ptr<ParamHandler>, N_KNOBS>>();
  for (size_t i = 0; i < n_pages(); i++) {
    Pane& pane = manager.get_pane(i);
    auto ary = std::array<std::unique_ptr<ParamHandler>, N_KNOBS>();
    for (size_t j = 0; j < N_KNOBS; j++) {
      ary[0] = std::make_unique<ParamHandler>(pane.get_param(static_cast<Knob>(j)));
    }
    knobs.push_back(std::move(ary));  // move because of unique_ptr
  }
  return source;
}

uint8_t FomeApp::n_pages() {
  return manager.n_panes();
}

KnobHandler& FomeApp::get_knob(uint8_t page, Knob knob) {
  return *knobs[page][knob];
}
