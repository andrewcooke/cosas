
#include <memory>

#include "engine.h"
#include "constants.h"
#include "console.h"


int main() {
  //  dump_w_gain(Manager::Engine::FM_FB, 0.1 * sample_rate);
  //  dump(Manager::Engine::CHORD, 0.1 * sample_rate);};
  //  dump(Manager::Engine::FM_SIMPLE, 0.1 * sample_rate);
  //  dump_w_gain(Manager::Engine::FM_SIMPLE, 0.1 * sample_rate);
  //  dump_w_wdex(Manager::Engine::FM_SIMPLE, 0.1 * sample_rate);
  dump(Manager::Engine::FM_ENV, sample_rate);
}
