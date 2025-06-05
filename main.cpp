
#include <memory>

#include "engine.h"
#include "modulators.h"
#include "constants.h"
#include "console.h"
//#include "wavetable.h"

int main() {
  //  dump_w_gain(Manager::Engine::FM_FB, 0.1 * sample_rate);
  dump(Manager::Engine::CHORD, 0.1 * sample_rate);
};
