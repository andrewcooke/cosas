
#ifndef COSAS_ENGINE_SMALL_H
#define COSAS_ENGINE_SMALL_H



#include <tuple>
#include <memory>

#include "cosas/engine_base.h"
#include "cosas/wavelib.h"
#include "cosas/oscillator.h"
#include "cosas/params.h"



class SmallManager : public BaseManager{

public:

  enum SmallEngine {
    POLY,
  };
  static constexpr size_t N_ENGINE = POLY + 1;

  SmallManager() = default;
  RelSource& build(SmallEngine);

private:

  RelSource& build_poly();
};


#endif

