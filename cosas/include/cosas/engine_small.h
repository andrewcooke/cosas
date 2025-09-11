
#ifndef COSAS_ENGINE_SMALL_H
#define COSAS_ENGINE_SMALL_H


#include "cosas/engine_base.h"



class SmallManager : public BaseManager {

public:

  enum SmallEngine {
    OSCILLATOR,
    SIMPLE_2_OSC_FM
  };
  static constexpr size_t N_ENGINE = SIMPLE_2_OSC_FM + 1;

  SmallManager() = default;
  PhaseSource& build(SmallEngine);

private:

  PhaseSource& build_oscillator();
  PhaseSource& build_simple_2_osc_fm();

};


#endif

